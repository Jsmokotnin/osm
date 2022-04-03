/**
 *  OSM
 *  Copyright (C) 2022  Pavel Smokotnin

 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "remoteclient.h"
#include "sourcelist.h"
#include "item.h"

namespace remote {

Client::Client(QObject *parent) : QObject(parent), m_network(),  m_thread(), m_timer(),
    m_sourceList(nullptr), m_servers(), m_items(), m_onRequest(false), m_updateCounter(0), m_needUpdate()
{
    connect(&m_network, &Network::datagramRecieved, this, &Client::dataRecieved);
    m_thread.setObjectName("NetworkClient");

    m_timer.setInterval(TIMER_INTERVAL);
    m_timer.moveToThread(&m_thread);
    connect(&m_timer, &QTimer::timeout, this, &Client::sendRequests, Qt::DirectConnection);
    connect(&m_thread, &QThread::started, this, [this]() {
        m_timer.start();
    }, Qt::DirectConnection);
    connect(&m_thread, &QThread::finished, this, [this]() {
        m_timer.stop();
    }, Qt::DirectConnection);
}

Client::~Client()
{
    stop();
}

void Client::setSourceList(SourceList *list)
{
    m_sourceList = list;
}

bool Client::start()
{
    m_network.bindUDP();
    m_network.joinMulticast();
    m_thread.start();
    return m_thread.isRunning();
}

void Client::stop()
{
    m_network.unbindUDP();
    m_thread.quit();
    m_thread.wait();
}

bool Client::active() const
{
    return m_thread.isRunning();
}

void Client::setActive(bool state)
{
    if (active() && !state) {
        stop();
    }
    if (!active() && state) {
        start();
    }
}

void Client::sendRequests()
{
    if (m_onRequest) {
        return;
    }

    auto result = std::min_element(m_needUpdate.begin(), m_needUpdate.end());

    if (result != m_needUpdate.end() && (*result) < ON_UPDATE) {
        (*result) = ON_UPDATE;
        m_onRequest = true;
        requestData(m_items[result.key()]);
    }
}

Item *Client::addItem(const QUuid &serverId, const QUuid &sourceId, const QString &host)
{
    auto item = new remote::Item(this);
    item->setServerId(serverId);
    item->setSourceId(sourceId);
    item->setHost(host);
    connect(item, &Item::updateData, this, &Client::requestUpdate);
    connect(item, &Item::destroyed, this, [ = ]() {
        m_items[qHash(sourceId)] = nullptr;
        m_needUpdate[qHash(sourceId)] = READY_FOR_UPDATE;
    }, Qt::DirectConnection);

    m_sourceList->appendItem(item);
    m_items[qHash(sourceId)] = item;

    return item;
}

void Client::dataRecieved(QHostAddress senderAddress, [[maybe_unused]] int senderPort, const QByteArray &data)
{
    auto document = QJsonDocument::fromJson(data);
    if (document.isNull()) {
        return;
    }

    auto serverId = QUuid::fromString(document["uuid"].toString());
    auto time = document["time"].toString();
    Q_UNUSED(time)

    if (document["message"].toString() == "hello") {
        auto port = document["port"].toInt();
        m_servers[qHash(serverId)] = {senderAddress, port};

        if (document["sources"].isArray()) {
            auto sources = document["sources"].toArray();
            for (int i = 0; i < sources.count(); i++) {
                auto sourceUuid = QUuid::fromString(sources[i].toString());
                if (m_items.find(qHash(sourceUuid)) == m_items.end()) {
                    auto item = addItem(serverId, sourceUuid, document["host"].toString());
                    requestChanged(item);
                }
            }
        }

        return;
    }

    if (!document["source"].toString().isNull()) {
        auto sourceId = QUuid::fromString(document["source"].toString());
        auto message = document["message"].toString();
        auto item = m_items.value(qHash(sourceId), nullptr);
        if (!m_sourceList) {
            return;
        }

        if (message == "added" && !item) {
            addItem(serverId, sourceId, document["host"].toString());
            message = "changed";
        }

        if (item && message == "removed") {
            m_sourceList->removeItem(item);
            m_items[qHash(sourceId)] = nullptr;
            m_needUpdate[qHash(sourceId)] = READY_FOR_UPDATE;
        }

        if (item && message == "changed") {
            requestChanged(item);
        }

        if (item && message == "readyRead") {
            requestUpdate(item);
        }
        return;
    }
}

void Client::requestUpdate(Item *item)
{
    if (item && item->active()) {
        if (m_needUpdate[qHash(item->sourceId())] == READY_FOR_UPDATE) {
            m_needUpdate[qHash(item->sourceId())] = ++m_updateCounter;
        }
    }
}

void Client::requestChanged(Item *item)
{
    Network::responseCallback onAnswer = [item](const QByteArray & data) {
        auto document = QJsonDocument::fromJson(data);
        auto jsonColor = document["color"].toObject();
        QColor c(
            jsonColor["red"  ].toInt(0),
            jsonColor["green"].toInt(0),
            jsonColor["blue" ].toInt(0),
            jsonColor["alpha"].toInt(1));
        item->setColor(c);
        item->setName(document["name"].toString());
        item->setOriginalActive(document["active"].toBool());
    };
    requestSource(item, "requestChanged", onAnswer);
}

void Client::requestData(Item *item)
{
    if (!item) {
        return;
    }
    Network::responseCallback onAnswer = [this, item](const QByteArray & data) {
        auto document = QJsonDocument::fromJson(data);
        auto ftData = document["ftdata"].toArray();
        item->applyData(ftData);
        m_needUpdate[qHash(item->sourceId())] = READY_FOR_UPDATE;
        m_onRequest = false;
    };
    Network::errorCallback onError = [this, item]() {
        item->setState(Item::ERROR);
        m_needUpdate[qHash(item->sourceId())] = READY_FOR_UPDATE;
        m_onRequest = false;
    };
    requestSource(item, "requestData", onAnswer, onError);
}

void Client::requestSource(Item *item, const QString &message, Network::responseCallback callback,
                           Network::errorCallback errorCallback)
{
    auto server = m_servers.value(qHash(item->serverId()), {{}, 0});
    if (server.first.isNull()) {
        return;
    }

    QJsonObject object;
    object["api"] = "Open Sound Meter";
    object["version"] = APP_GIT_VERSION;
    object["message"] = message;
    object["uuid"] = item->sourceId().toString();
    QJsonDocument document(object);
    auto data = document.toJson(QJsonDocument::JsonFormat::Compact);

    Network::errorCallback onError = [item]() {
        qDebug() << "onError";
        item->setState(Item::ERROR);
        item->setActive(false);
    };
    m_network.sendTCP(data, server.first.toString(), server.second, callback, errorCallback ? errorCallback : onError);
}

} // namespace remote