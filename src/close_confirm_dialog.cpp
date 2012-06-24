/**
 * \file
 *
 * \brief Class \c kate::CloseConfirmDialog (implementation)
 *
 * \date Sun Jun 24 16:29:13 MSK 2012 -- Initial design
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
#include <src/close_confirm_dialog.h>

// Standard includes
#include <KConfig>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <KVBox>
#include <QtGui/QLabel>
#include <QtGui/QHeaderView>
#include <cassert>

namespace kate { namespace {

class KateDocItem : public QTreeWidgetItem
{
  public:
    KateDocItem(KTextEditor::Document* doc, QTreeWidget* tw)
      : QTreeWidgetItem(tw)
      , document(doc)
    {
      setText(0, doc->url().prettyUrl());
      setCheckState(0, Qt::Checked);
    }
    KTextEditor::Document* document;
};
}                                                           // anonymous namespace

CloseConfirmDialog::CloseConfirmDialog(
    QList<KTextEditor::Document*>& docs
  , QWidget* const parent
  )
  : KDialog(parent)
  , m_docs(docs)
{
    assert("Documents container expected to be non empty" && !docs.isEmpty());

    setCaption(i18n("Close files confirmation"));
    setButtons(Ok | Cancel);
    setModal(true);
    setDefaultButton(KDialog::Ok);

    KVBox* w = new KVBox(this);
    setMainWidget(w);
    w->setSpacing(KDialog::spacingHint());

    KHBox* lo1 = new KHBox(w);

    // dialog text
    QLabel* icon = new QLabel(lo1);
    icon->setPixmap(DesktopIcon("dialog-warning"));

    QLabel* t = new QLabel(
        i18n(
            "<qt>You are about to close the following documents..."
            "<p>Uncheck those you don't want to close.</p></qt>"
          )
      , lo1
      );
    lo1->setStretchFactor(t, 1000);

    // document list
    m_docs_tree = new QTreeWidget(w);
    QStringList header;
    header << i18n("Filename");
    m_docs_tree->setHeaderLabels(header);
    m_docs_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_docs_tree->setRootIsDecorated(false);
    for (int i = 0; i < m_docs.size(); i++)
    {
        new KateDocItem(m_docs[i], m_docs_tree);
    }
    m_docs_tree->header()->setStretchLastSection(false);
    m_docs_tree->header()->setResizeMode(0, QHeaderView::Stretch);
    m_docs_tree->header()->setResizeMode(1, QHeaderView::ResizeToContents);

    // Update documents list according checkboxes
    connect(this, SIGNAL(accepted()), this, SLOT(updateDocsList()));

    KConfigGroup gcg(KGlobal::config(), "CLoseConfirmationDialog");
    restoreDialogSize(gcg);                                 // restore dialog geometry from config
}

CloseConfirmDialog::~CloseConfirmDialog()
{
    KConfigGroup gcg(KGlobal::config(), "CLoseConfirmationDialog");
    saveDialogSize(gcg);                                    // write dialog geometry to config
    gcg.sync();
}

/**
 * Going to remove unchecked files from the given documents list
 */
void CloseConfirmDialog::updateDocsList()
{
    for (
        QTreeWidgetItemIterator it(m_docs_tree, QTreeWidgetItemIterator::NotChecked)
      ; *it
      ; ++it
      )
    {
        KateDocItem* item = static_cast<KateDocItem*>(*it);
        m_docs.removeAll(item->document);
        kDebug() << "do not close the file " << item->document->url().prettyUrl();
    }
}

}                                                           // namespace kate
