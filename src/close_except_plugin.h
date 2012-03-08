/**
 * \file
 *
 * \brief Class \c kate::CloseExceptPlugin (interface)
 *
 * \date Thu Mar  8 08:13:43 MSK 2012 -- Initial design
 */
/*
 * KateCloseExceptPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KateCloseExceptPlugin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SRC__CLOSE_EXCEPT_PLUGIN_H__
#  define __SRC__CLOSE_EXCEPT_PLUGIN_H__

// Project specific includes

// Standard includes
#  include <kate/plugin.h>
#  include <kate/pluginconfigpageinterface.h>
#  include <KTextEditor/Document>
#  include <KTextEditor/View>
#  include <cassert>

namespace kate {
class CloseExceptPlugin;                                    // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class CloseExceptPluginView
  : public Kate::PluginView
  , public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    /// Default constructor
    CloseExceptPluginView(Kate::MainWindow*, const KComponentData&, CloseExceptPlugin*);
    /// Destructor
    virtual ~CloseExceptPluginView();

private Q_SLOTS:
    void closeExcept(const QString&);
};

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class CloseExceptPlugin : public Kate::Plugin
{
    Q_OBJECT

public:
    /// Default constructor
    CloseExceptPlugin(QObject* = 0, const QList<QVariant>& = QList<QVariant>());
    /// Destructor
    virtual ~CloseExceptPlugin() {}
    /// Create a new view of this plugin for the given main window
    Kate::PluginView *createView(Kate::MainWindow*);
};

}                                                           // namespace kate
#endif                                                      // __SRC__CLOSE_EXCEPT_PLUGIN_H__
