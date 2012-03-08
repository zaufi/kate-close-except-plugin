/**
 * \file
 *
 * \brief Class \c kate::CloseExceptPlugin (implementation)
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

// Project specific includes
#include <src/config.h>
#include <src/close_except_plugin.h>

// Standard includes
#include <kate/application.h>
#include <kate/mainwindow.h>
#include <KAboutData>
#include <KActionCollection>
#include <KDebug>
#include <KPluginFactory>
#include <KPluginLoader>

K_PLUGIN_FACTORY(CloseExceptPluginFactory, registerPlugin<kate::CloseExceptPlugin>();)
K_EXPORT_PLUGIN(
    CloseExceptPluginFactory(
        KAboutData(
            "katecloseexceptplugin"
          , "kate_closeexcept_plugin"
          , ki18n("Close documents depending on path")
          , PLUGIN_VERSION
          , ki18n("Close all documents started from specified path")
          , KAboutData::License_LGPL_V3
          )
      )
  )

namespace kate {
//BEGIN CloseExceptPlugin
CloseExceptPlugin::CloseExceptPlugin(
    QObject* application
  , const QList<QVariant>&
  )
  : Kate::Plugin(static_cast<Kate::Application*>(application), "kate_closeexcept_plugin")
{
}

Kate::PluginView* CloseExceptPlugin::createView(Kate::MainWindow* parent)
{
    return new CloseExceptPluginView(parent, CloseExceptPluginFactory::componentData(), this);
}
//END CloseExceptPlugin

//BEGIN CloseExceptPluginView
CloseExceptPluginView::CloseExceptPluginView(Kate::MainWindow* mw, const KComponentData& data, CloseExceptPlugin*)
  : Kate::PluginView(mw)
  , Kate::XMLGUIClient(data)
{
    mainWindow()->guiFactory()->addClient(this);
}

CloseExceptPluginView::~CloseExceptPluginView() {
    mainWindow()->guiFactory()->removeClient(this);
}

void CloseExceptPluginView::closeExcept(const QString&)
{

}
//END CloseExceptPluginView
}                                                           // namespace kate
