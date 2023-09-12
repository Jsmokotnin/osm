/**
 *  OSM
 *  Copyright (C) 2023  Pavel Smokotnin

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

#ifndef META_WINDOWING_H
#define META_WINDOWING_H

#include <QObject>
#include <QtQml>

#include "metabase.h"
#include "chart/source.h"
#include "math/windowfunction.h"

namespace meta {

class Windowing : public Base
{
    Q_GADGET
    QML_ELEMENT

    //Q_PROPERTY(float wide READ wide WRITE setWide NOTIFY wideChanged)
    //Q_PROPERTY(float offset READ offset WRITE setOffset NOTIFY offsetChanged)
    //Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    //Q_PROPERTY(QString tipName READ tipName WRITE setTipName NOTIFY tipNameChanged)
    //Q_PROPERTY(WindowFunction::Type windowFunctionType READ windowFunctionType WRITE setWindowFunctionType NOTIFY windowFunctionTypeChanged)
    //Q_PROPERTY(SourceDomain domain READ domain WRITE setDomain NOTIFY domainChanged)

public:
    enum Mode {
        FFT8, FFT9, FFT10, FFT11, FFT12, FFT13, FFT14, FFT15, FFT16
    };
    Q_ENUM(Mode)
    enum SourceDomain {
        Time, Frequency
    };
    Q_ENUM(SourceDomain)

    Windowing();

    static QVariant getAvailableModes();
    static QVariant getAvailableWindowTypes();

    static const std::map<Mode, QString> m_modeMap;
    static const std::map<Mode, int> m_FFTsizes;

    float wide() const;
    void setWide(float newWide);

    float offset() const;
    void setOffset(float newOffset);

    Mode mode() const;
    void setMode(Mode newMode);
    void setMode(QVariant newMode);

    const QString &tipName() const;
    void setTipName(const QString &newTipName);

    const WindowFunction::Type &windowFunctionType() const;
    void setWindowFunctionType(const QVariant &newWindowFunctionType);
    void setWindowFunctionType(const WindowFunction::Type &newWindowFunctionType);

    meta::Windowing::SourceDomain domain() const;
    void setDomain(QVariant newDomain);
    void setDomain(SourceDomain newDomain);

    Q_INVOKABLE virtual chart::Source *store() noexcept = 0;

//virtual signals:
    virtual void wideChanged(float) = 0;
    virtual void offsetChanged(float) = 0;
    virtual void modeChanged(Mode) = 0;
    virtual void tipNameChanged(QString) = 0;
    virtual void windowFunctionTypeChanged(WindowFunction::Type) = 0;
    virtual void domainChanged(meta::Windowing::SourceDomain) = 0;

protected:
    float                   m_wide;
    float                   m_offset;
    QString                 m_tipName;
    Mode                    m_mode;
    SourceDomain            m_domain;
    WindowFunction::Type    m_windowFunctionType;
};

} // namespace meta

#endif // META_WINDOWING_H
