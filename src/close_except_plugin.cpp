/**
 * \file
 *
 * \brief Kate Close Except/Like plugin implementation
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
#include <src/close_confirm_dialog.h>

// Standard includes
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <KAboutData>
#include <KActionCollection>
#include <KDebug>
#include <KPassivePopup>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KTextEditor/Editor>
#include <QtCore/QFileInfo>

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

void CloseExceptPlugin::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup scg(config, groupPrefix + "menu");
    m_show_confirmation_needed = scg.readEntry("ShowConfirmation", true);
    kDebug() << "READ SESSION CONFIG: sc=" << m_show_confirmation_needed;
}

void CloseExceptPlugin::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug() << "WRITE SESSION CONFIG: sc=" << m_show_confirmation_needed;
    KConfigGroup scg(config, groupPrefix + "menu");
    scg.writeEntry("ShowConfirmation", m_show_confirmation_needed);
    scg.sync();
}
//END CloseExceptPlugin

//BEGIN CloseExceptPluginView
CloseExceptPluginView::CloseExceptPluginView(
    Kate::MainWindow* mw
  , const KComponentData& data
  , CloseExceptPlugin* plugin
  )
  : Kate::PluginView(mw)
  , Kate::XMLGUIClient(data)
  , m_plugin(plugin)
  , m_show_confirmation_action(new KToggleAction(i18n("Show Confirmation"), this))
  , m_except_menu(new KActionMenu(i18n("Close Except"), this))
  , m_like_menu(new KActionMenu(i18n("Close Like"), this))
{
    actionCollection()->addAction("file_close_except", m_except_menu);
    actionCollection()->addAction("file_close_like", m_like_menu);

    // Subscribe self to document creation
    connect(
        m_plugin->application()->editor()
      , SIGNAL(documentCreated(KTextEditor::Editor*, KTextEditor::Document*))
      , this
      , SLOT(documentCreated(KTextEditor::Editor*, KTextEditor::Document*))
      );
    // Configure toggle action and connect it to update state
    m_show_confirmation_action->setChecked(m_plugin->showConfirmationNeeded());
    connect(
        m_show_confirmation_action
      , SIGNAL(toggled(bool))
      , m_plugin
      , SLOT(toggleShowConfirmation(bool))
      );
    // Fill menu w/ currently opened document masks/groups
    updateMenu();

    mainWindow()->guiFactory()->addClient(this);
}

CloseExceptPluginView::~CloseExceptPluginView() {
    mainWindow()->guiFactory()->removeClient(this);
}

void CloseExceptPluginView::documentCreated(KTextEditor::Editor*, KTextEditor::Document* document)
{
    kDebug() << "A new document opened " << document->url();

    // Subscribe self to document close and name changes
    connect(
        document
      , SIGNAL(aboutToClose(KTextEditor::Document*))
      , this
      , SLOT(updateMenuSlotStub(KTextEditor::Document*))
      );
    connect(
        document
      , SIGNAL(documentNameChanged(KTextEditor::Document*))
      , this
      , SLOT(updateMenuSlotStub(KTextEditor::Document*))
      );
}

void CloseExceptPluginView::updateMenuSlotStub(KTextEditor::Document*)
{
    updateMenu();
}

void CloseExceptPluginView::appendActionsFrom(
    const std::set<QString>& paths
  , actions_map_type& actions
  , KActionMenu* menu
  , QSignalMapper* mapper
  )
{
    Q_FOREACH(const QString& path, paths)
    {
        QString action = path.startsWith("*") ? path : path + "*";
        actions[action] = QPointer<KAction>(new KAction(action, menu));
        menu->addAction(actions[action]);
        connect(actions[action], SIGNAL(triggered()), mapper, SLOT(map()));
        mapper->setMapping(actions[action], action);
    }
}

QPointer<QSignalMapper> CloseExceptPluginView::updateMenu(
    const std::set<QString>& paths
  , const std::set<QString>& masks
  , actions_map_type& actions
  , KActionMenu* menu
  )
{
    // turn menu ON or OFF depending on collected results
    menu->setEnabled(!paths.empty());

    // Clear previous menus
    for (actions_map_type::iterator it = actions.begin(), last = actions.end(); it !=last;)
    {
        menu->removeAction(*it);
        actions.erase(it++);
    }
    // Form a new one
    QPointer<QSignalMapper> mapper = QPointer<QSignalMapper>(new QSignalMapper(this));
    appendActionsFrom(paths, actions, menu, mapper);
    if (!masks.empty())
    {
        if (!paths.empty())
            menu->addSeparator();                           // Add separator between paths and file's ext filters
        appendActionsFrom(masks, actions, menu, mapper);
    }
    // Append 'Show Confirmation' toggle menu item
    menu->addSeparator();                                   // Add separator between paths and show confirmation
    menu->addAction(m_show_confirmation_action);
    return mapper;
}

void CloseExceptPluginView::updateMenu()
{
    kDebug() << "... updating menu ...";
    const QList<KTextEditor::Document*>& docs = m_plugin->application()->documentManager()->documents();
    if (docs.empty())
    {
        kDebug() << "No docs r opened right now --> disable menu";
        m_except_menu->setEnabled(false);
        m_like_menu->setEnabled(false);
        /// \note It seems there is always a document present... it named \em 'Untitled'
    }
    else
    {
        // Iterate over documents and form a set of candidates
        typedef std::set<QString> paths_set_type;
        paths_set_type paths;
        paths_set_type masks;
        Q_FOREACH(KTextEditor::Document* document, docs)
        {
            const QString& ext = QFileInfo(document->url().path()).completeSuffix();
            if (!ext.isEmpty())
                masks.insert("*." + ext);
            for (
                KUrl url = document->url().upUrl()
              ; url.hasPath() && url.path() != "/"
              ; url = url.upUrl()
              ) paths.insert(url.path());
        }
        // Remove common entries -- i.e. such that every path in a list
        // starts w/ it... so final menu length will be decrased a little.
        for (paths_set_type::iterator it = paths.begin(), last = paths.end(); it != last;)
        {
            paths_set_type::iterator not_it = paths.begin();
            for (; not_it != last; ++not_it)
                if (!not_it->startsWith(*it))
                    break;
            if (not_it == last)
                paths.erase(it++);
            else
                ++it;
        }
        //
        m_except_mapper = updateMenu(paths, masks, m_except_actions, m_except_menu);
        m_like_mapper = updateMenu(paths, masks, m_like_actions, m_like_menu);
        connect(m_except_mapper, SIGNAL(mapped(const QString&)), this, SLOT(closeExcept(const QString&)));
        connect(m_like_mapper, SIGNAL(mapped(const QString&)), this, SLOT(closeLike(const QString&)));
    }
}

void CloseExceptPluginView::close(const QString& item, const bool close_if_match)
{
    assert(
        "Parameter seems invalid! Is smth changed in the code?"
      && !item.isEmpty() && (item[0] == '*' || item[item.size() - 1] == '*')
      );

    const bool is_path = item[0] != '*';
    const QString mask = is_path ? item.left(item.size() - 1) : item;
    kDebug() << "Going to close items [" << close_if_match << "/" << is_path << "]: " << mask;

    QList<KTextEditor::Document*> docs2close;
    const QList<KTextEditor::Document*>& docs = m_plugin->application()->documentManager()->documents();
    Q_FOREACH(KTextEditor::Document* document, docs)
    {
        const QString& path = document->url().upUrl().path();
        const QString& ext = QFileInfo(document->url().fileName()).completeSuffix();
        const bool match = (!is_path && mask.endsWith(ext))
          || (is_path && path.startsWith(mask))
          ;
        if (match == close_if_match)
        {
            kDebug() << "*** Will close: " << document->url();
            docs2close.push_back(document);
        }
    }
    if (docs2close.isEmpty())
    {
        KPassivePopup::message(
            i18n("Oops")
          , i18n("No files to close ...")
          , qobject_cast<QWidget*>(this)
          );
        return;
    }
    // Show confirmation dialog if needed
    const bool removeNeeded = !m_plugin->showConfirmationNeeded()
      || CloseConfirmDialog(docs2close, m_show_confirmation_action, qobject_cast<QWidget*>(this)).exec();
    if (removeNeeded)
    {
        if (docs2close.isEmpty())
        {
            KPassivePopup::message(
                i18n("Oops")
            , i18n("No files to close ...")
            , qobject_cast<QWidget*>(this)
            );
        }
        else
        {
            // Close 'em all!
            m_plugin->application()->documentManager()->closeDocumentList(docs2close);
            updateMenu();
            KPassivePopup::message(
                i18n("Done")
              , i18np("%1 file closed", "%1 files closed", docs2close.size())
              , qobject_cast<QWidget*>(this)
              );
        }
    }
}
//END CloseExceptPluginView
}                                                           // namespace kate
