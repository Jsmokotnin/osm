/**
 *  OSM
 *  Copyright (C) 2018  Pavel Smokotnin

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
#include "variablechart.h"
#include "rtaplot.h"
#include "magnitudeplot.h"
#include "phaseplot.h"
#include "impulseplot.h"
#include "stepplot.h"
#include "coherenceplot.h"
#include "groupdelayplot.h"
#include "spectrogramplot.h"
#include "src/sourcelist.h"

using namespace Fftchart;

VariableChart::VariableChart(QQuickItem *parent) :
    QQuickItem(parent),
    m_plot(nullptr),
    m_sources(nullptr),
    m_settings(nullptr),
    m_selected(RTA)
{
    setFlag(QQuickItem::ItemHasContents);
    initType();
}
void VariableChart::initType()
{
    Plot *newPlot = nullptr;
    switch (m_selected) {
    case RTA:
        newPlot = new RTAPlot(m_settings, this);
        break;

    case Magnitude:
        newPlot = new MagnitudePlot(m_settings, this);
        break;

    case Phase:
        newPlot = new PhasePlot(m_settings, this);
        break;

    case Impulse:
        newPlot = new ImpulsePlot(m_settings, this);
        break;

    case Step:
        newPlot = new StepPlot(m_settings, this);
        break;

    case Coherence:
        newPlot = new CoherencePlot(m_settings, this);
        break;

    case GroupDelay:
        newPlot = new GroupDelayPlot(m_settings, this);
        break;
    case Spectrogram:
        newPlot = new SpectrogramPlot(m_settings, this);
        break;
    default:
        return;
    }
    newPlot->setParentItem(this);
    newPlot->setDarkMode(darkMode());

    if (m_plot) {
        if (!qobject_cast<SpectrogramPlot *>(m_plot)) {
            newPlot->setFilter(m_plot->filter());
        }
        m_plot->clear();
        m_plot->disconnectFromParent();
        m_plot->setParent(nullptr);
        m_plot->setParentItem(nullptr);
        m_plot->deleteLater();
        newPlot->storeSettings();
    }

    m_plot = newPlot;
}
QString VariableChart::typeString() const
{
    return typeMap.at(m_selected);
}
void VariableChart::setType(const Type &type)
{
    if (m_selected != type) {
        m_selected = type;
        initType();
        if (m_sources) {
            for (int i = 0; i < m_sources->count(); ++i) {
                appendDataSource(m_sources->items()[i]);
            }
        }
        updateZOrders();
        emit typeChanged();
    }
}
void VariableChart::setTypeByString(const QString &type)
{
    for (auto &it : typeMap) {
        if (it.second == type) {
            setType(it.first);
        }
    }
}
void VariableChart::setSettings(Settings *settings) noexcept
{
    m_settings = settings;
    m_plot->setSettings(m_settings);
}
void VariableChart::appendDataSource(Source *source)
{
    if (m_plot) {
        m_plot->appendDataSource(source);
    }
}
void VariableChart::removeDataSource(Source *source)
{
    if (m_plot) {
        m_plot->removeDataSource(source);
    }
}

void VariableChart::setSourceZIndex(Source *source, int index)
{
    if (m_plot) {
        m_plot->setSourceZIndex(source, index);
    }
}

void VariableChart::setSources(SourceList *sourceList)
{
    if (m_sources == sourceList)
        return;

    if (m_sources)
        m_sources->disconnect(this);

    m_sources = sourceList;

    if (m_sources) {
        for (int i = 0; i < m_sources->count(); ++i) {
            appendDataSource(m_sources->items()[i]);
        }

        connect(m_sources, &SourceList::postItemAppended, this, [ = ](Fftchart::Source * source) {
            appendDataSource(source);
        });

        connect(m_sources, &SourceList::preItemRemoved, this, [ = ](int index) {
            auto source = m_sources->get(index);
            removeDataSource(source);
        });

        connect(m_sources, &SourceList::postItemMoved, this, &VariableChart::updateZOrders);

        connect(m_sources, &SourceList::selectedChanged, this, [this]() {
            if (m_plot) {
                updateZOrders();
                auto selected = m_sources->selected();
                m_plot->setHighlighted(selected);
                setSourceZIndex(selected, m_sources->count() + 1);
            }
        });
    }
    updateZOrders();
    emit sourcesChanged();
}

void VariableChart::setDarkMode(bool darkMode) noexcept
{
    m_darkMode = darkMode;
    if (m_plot) {
        m_plot->setDarkMode(darkMode);
    }
}

void VariableChart::updateZOrders() noexcept
{
    if (!m_plot || !m_sources) {
        return ;
    }
    auto total = m_sources->count();
    for (auto &&source : *m_sources) {
        auto z = total - m_sources->indexOf(source);
        setSourceZIndex(source, z);
    }
    update();
}
