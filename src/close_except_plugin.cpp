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
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <KAboutData>
#include <KActionCollection>
#include <KDebug>
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
  , m_menu(new KActionMenu(i18n("Close Except"), this))
{
    actionCollection()->addAction("file_close_except", m_menu);

    // Subscribe self to document creation
    connect(
        m_plugin->application()->editor()
      , SIGNAL(documentCreated(KTextEditor::Editor*, KTextEditor::Document*))
      , this
      , SLOT(documentCreated(KTextEditor::Editor*, KTextEditor::Document*))
      );

    mainWindow()->guiFactory()->addClient(this);
    // Fill menu w/ currently opened document masks/groups
    updateMenu();
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

void CloseExceptPluginView::appendActionsFrom(const std::set<QString>& paths)
{
    Q_FOREACH(const QString& path, paths)
    {
        m_actions[path] = QPointer<KAction>(new KAction(path, m_menu));
        m_menu->addAction(m_actions[path]);
        connect(m_actions[path], SIGNAL(triggered()), m_mapper, SLOT(map()));
        m_mapper->setMapping(m_actions[path], path);
    }
}

void CloseExceptPluginView::updateMenu()
{
    kDebug() << "... updating menu ...";
    const QList<KTextEditor::Document*>& docs = m_plugin->application()->documentManager()->documents();
    if (docs.empty())
    {
        kDebug() << "No docs r opened right now --> disable menu";
        m_menu->setEnabled(false);
        /// \note It seems there is always a document present... it named \em 'Untitled'
    }
    else
    {
        // Iterate over documents and form a set of candidates
        std::set<QString> candidates;
        std::set<QString> masks;
        Q_FOREACH(KTextEditor::Document* document, docs)
        {
            const QString& ext = QFileInfo(document->url().path()).completeSuffix();
            if (!ext.isEmpty())
                masks.insert("*." + ext);
            for (
                KUrl url = document->url().upUrl()
              ; url.hasPath() && url.path() != "/"
              ; url = url.upUrl()
              ) candidates.insert(url.path() + "*");
        }
        // turn 'Close Except...' menu ON or OFF depending on collected results
        m_menu->setEnabled(!candidates.empty());

        // Clear previous menu
        for (actions_map_type::iterator it = m_actions.begin(), last = m_actions.end(); it !=last;)
        {
            m_menu->removeAction(*it);
            m_actions.erase(it++);
        }
        // Form a new one
        m_mapper = QPointer<QSignalMapper>(new QSignalMapper(this));
        appendActionsFrom(candidates);
        if (!masks.empty())
        {
            if (!candidates.empty())
                m_menu->addSeparator();                     // Add separator between paths and file's ext filters
            appendActionsFrom(masks);
        }
        connect(m_mapper, SIGNAL(mapped(const QString&)), this, SLOT(closeExcept(const QString&)));
    }
}

void CloseExceptPluginView::close(const QString& item, const bool close_if_match)
{
    assert("Expect non empty parameter" && item.isEmpty());
    assert(
        "Parameter seems invalid! Is smth changed in the code?"
      && (item.front() == '*' || item.back() == '*')
      );
    kDebug() << "Going to close items [" << close_if_match << "]: " << item;

    const bool is_path = item[0] != '*';
    QList<KTextEditor::Document*> docs2close;
    const QList<KTextEditor::Document*>& docs = m_plugin->application()->documentManager()->documents();
    Q_FOREACH(KTextEditor::Document* document, docs)
    {
        const QString& path = document->url().upUrl().path();
        const QString& ext = QFileInfo(document->url().fileName()).completeSuffix();
        const bool match = (is_path && item.startsWith(path)) || (!is_path && item.endsWith(ext));
        if (match == close_if_match)
        {
            kDebug() << "*** Will close: " << document->url();
            docs2close.push_back(document);
        }
    }
    // Close 'em all!
    m_plugin->application()->documentManager()->closeDocumentList(docs2close);
    updateMenu();
}
//END CloseExceptPluginView
}                                                           // namespace kate
