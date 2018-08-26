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
import QtQuick 2.7

MouseArea {
    property string propertiesQml
    property var pushObject

    anchors.fill: parent
    onClicked: {
        var aw = applicationWindow;
        aw.properiesbar.stack.clear();

        if (propertiesQml) {
            var item = aw.properiesbar.stack.push(
                    propertiesQml,
                    {
                        dataObject: pushObject
                    }
            )
        }
        else
            console.error("qml not set for ", parent.parent)
    }
}
