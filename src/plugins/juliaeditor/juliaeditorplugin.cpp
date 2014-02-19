#include "juliaeditorplugin.h"
#include "juliaeditor_constants.h"
#include "juliaeditor.h"
#include "juliasettingspage.h"
#include "singleton.h"
#include "juliaruncontrolfactory.h"
#include "juliarunconfigurationfactory.h"
#include "juliadummyproject.h"
#include "juliaprojectmanager.h"
#include "juliaconsolepane.h"
#include "localevaluator.h"
#include "localtcpevaluator.h"
#include "juliafilewizard.h"
#include "commandhistoryview.h"
#include "packagecontroller.h"
#include "juliacompletionassist.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/basefilewizard.h>
#include <texteditor/texteditorplugin.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QAction>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#else
#include <QtGui/QAction>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#endif

#include <QtCore/QtPlugin>
#include <QCoreApplication>

using namespace JuliaPlugin;
using namespace JuliaPlugin::Internal;

// JuliaEditorPlugin *******

JuliaEditorPlugin::JuliaEditorPlugin()
  : action_handler(NULL), evaluator(NULL), console_pane(NULL), load_action(NULL)
{}

JuliaEditorPlugin::~JuliaEditorPlugin()
{
  // Unregister objects from the plugin manager's object pool
  // Delete members

  if (console_pane)
    ExtensionSystem::PluginManager::removeObject(console_pane);

  delete action_handler;
}

bool JuliaEditorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
  Q_UNUSED(arguments)
  Q_UNUSED(errorString)

  // "In the initialize method, a plugin can be sure that the plugins it
  //  depends on have initialized their members."

  // Types and settings -------
  if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/juliaeditor/juliaeditor.mimetypes.xml"), errorString))
      return false;

  Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
  iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/juliaeditor/images/jlfile.png")), "jl");


  addAutoReleasedObject( new JuliaSettingsPage() );
  Singleton<JuliaSettings>::GetInstance()->FromSettings(Core::ICore::settings());
  // ------- */

  // Editors -------
  //JuliaProjectManager* project_manager = new JuliaProjectManager();
  //addAutoReleasedObject( project_manager );
  //addAutoReleasedObject(new JuliaRunConfigurationFactory());
  //addAutoReleasedObject(new JuliaRunControlFactory());

  JuliaEditorFactory* editor_factory = new JuliaEditorFactory(this);
  connect( editor_factory, SIGNAL(newEditor(JuliaEditorWidget*)), SLOT(initEditor(JuliaEditorWidget*)) );
  addAutoReleasedObject( editor_factory );

  action_handler = new TextEditor::TextEditorActionHandler( Constants::JULIAEDITOR,
    TextEditor::TextEditorActionHandler::Format
    | TextEditor::TextEditorActionHandler::UnCommentSelection
    | TextEditor::TextEditorActionHandler::UnCollapseAll
    | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor );

  action_handler->initializeActions();
  // ------- */

  // File wizard -------
  QObject *core = Core::ICore::instance();
  Core::BaseFileWizardParameters JuliaWizardParameters(Core::IWizard::FileWizard);
  JuliaWizardParameters.setCategory(QLatin1String("Julia"));
  JuliaWizardParameters.setDisplayCategory(QLatin1String("Julia"));
  JuliaWizardParameters.setDescription(tr("Creates a Julia file."));
  JuliaWizardParameters.setDisplayName(tr("Julia File"));
  JuliaWizardParameters.setId(QLatin1String("Julia.Julia"));
  JuliaWizardParameters.setExtended(false);
  addAutoReleasedObject(new JuliaFileWizard(JuliaWizardParameters, core));

  // ------- */

  // Julia console -------
  evaluator = new LocalTcpEvaluator(this);
  console_pane = new JuliaConsolePane(this);
  Console* console = console_pane->outputWidget();
  Core::EditorManager* editor_manager = Core::EditorManager::instance();

  connect( evaluator, SIGNAL( ready() ), console, SLOT( BeginCommand() ) );
  connect( console, SIGNAL( NewCommand(ProjectExplorer::EvaluatorMessage) ), evaluator, SLOT( eval(const ProjectExplorer::EvaluatorMessage&) ) );
  connect( evaluator, SIGNAL( output(const ProjectExplorer::EvaluatorMessage*) ), console, SLOT( DisplayMsg(const ProjectExplorer::EvaluatorMessage*) ) );
  connect( evaluator, SIGNAL( output(const QString&) ), console, SLOT( DisplayMsg(const QString&) ) );

  evaluator->startup();

  connect( console, SIGNAL( Resetting(bool) ), evaluator, SLOT( reset() ) );
  connect( Singleton<JuliaSettings>::GetInstance(), SIGNAL(PathToBinariesChanged(const QString&,const QString&)), console, SLOT(Reset()) );
  connect( editor_manager, SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(currEditorChanged(Core::IEditor*)) );
  connect(ProjectExplorer::ProjectExplorerPlugin::instance()->session(), SIGNAL(sessionLoaded(QString)), this, SLOT(sessionLoaded()));

//  Core::IEditor* curr_editor = editor_manager->currentEditor();
//  if (curr_editor)
//    evaluator->setWorkingDir(QFileInfo( curr_editor->document()->fileName() ).absoluteDir().absolutePath());
//  else
//    evaluator->setWorkingDir(QDir::currentPath());

  addAutoReleasedObject(evaluator);
  ExtensionSystem::PluginManager::addObject(console_pane);
  // ------- */

  addAutoReleasedObject(new JuliaCompletionAssistProvider(evaluator));

  // Navigation Menus -------
  addAutoReleasedObject( new CommandHistoryViewFactory( console_pane->getConsoleHandle() ) );

  PackageController* package_controller = new PackageController(evaluator, console);
  PackageViewFactory* package_view_factory = new PackageViewFactory;
  connect( package_view_factory, SIGNAL(createdWidget(Core::NavigationView*)), package_controller, SLOT(OnNewPackageView(Core::NavigationView*) ) );
  connect( console, SIGNAL(Resetting()), package_controller, SLOT(OnConsoleReset() ) );

  addAutoReleasedObject(package_controller);
  addAutoReleasedObject(package_view_factory);
  // ------- */

  Core::ActionManager *am = Core::ICore::instance()->actionManager();

  // Menu -------
  Core::ActionContainer *menu = am->createMenu(Constants::MENU_ID);
  menu->menu()->setTitle(tr("Julia"));
  am->actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);
  // ------- */

  // Actions -------
  QAction *action = new QAction(tr("Reset Console"), this);
  Core::Command *cmd = am->registerAction(action, Constants::ACTION_ID_RESET_CONSOLE, Core::Context(Core::Constants::C_GLOBAL));
  cmd->setDefaultKeySequence(Qt::Key_F3);
  connect(action, SIGNAL(triggered()), console, SLOT(Reset()));
  menu->addAction(cmd);

  action = new QAction( tr("Clear Console"), this );
  cmd = am->registerAction( action, Constants::ACTION_ID_CLEAR_CONSOLE, Core::Context(Core::Constants::C_GLOBAL));
  cmd->setDefaultKeySequence(Qt::Key_F4);
  connect(action, SIGNAL(triggered()), console_pane, SLOT(clearContents()));
  menu->addAction(cmd);

  cmd = am->command( ProjectExplorer::Constants::RUN );
  load_action = cmd->action();
  connect( load_action, SIGNAL(triggered()), SLOT(evalCurrFile()) );
  connect( Core::EditorManager::instance(), SIGNAL(editorOpened(Core::IEditor*)), SLOT(updateLoadAction()) );
  connect( Core::EditorManager::instance(), SIGNAL(editorsClosed(QList<Core::IEditor*>)), SLOT(updateLoadAction()) );
  updateLoadAction();
  menu->addAction(cmd);
  // ------- */
  
  return true;
}

void JuliaEditorPlugin::extensionsInitialized()
{
  // Retrieve objects from the plugin manager's object pool
  // "In the extensionsInitialized method, a plugin can be sure that all
  //  plugins that depend on it are completely initialized."
}

ExtensionSystem::IPlugin::ShutdownFlag JuliaEditorPlugin::aboutToShutdown()
{
  // Save settings
  // Disconnect from signals that are not needed during shutdown
  // Hide UI (if you add UI that is not in the main window directly)
  return SynchronousShutdown;
}

void JuliaEditorPlugin::initEditor( JuliaEditorWidget* editor )
{
  action_handler->setupActions( editor );  // setupActions should be a slot!
  TextEditor::TextEditorSettings::instance()->initializeEditor( editor );
}

void JuliaEditorPlugin::evalCurrFile()
{
  // popup regardless, the user obviously wants to execute some code
  console_pane->popup( Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus );

  Core::IEditor* editor = Core::EditorManager::currentEditor();
  if ( editor )
  {
    Core::EditorManager* manager = Core::EditorManager::instance();
    foreach( Core::IEditor* editor, manager->openedEditors() )
    {
      if ( editor->document()->isModified() )
        manager->saveEditor( editor );
    }

    Core::IDocument* document = editor->document();
    QFileInfo file_info(document->fileName());

    console_pane->outputWidget()->SetBusy(file_info.baseName());
    evaluator->eval(&file_info);
  }
}

void JuliaEditorPlugin::updateLoadAction()
{
  Core::EditorManager* manager = Core::EditorManager::instance();
  load_action->setEnabled( manager->openedEditors().size() );
}

void JuliaEditorPlugin::currEditorChanged(Core::IEditor *editor)
{
  if ( !editor || !evaluator )
    return;

  QString file_name = editor->document()->fileName();
  if (file_name.size())
    evaluator->setWorkingDir( QFileInfo( file_name ).absoluteDir().absolutePath() );
}

void JuliaEditorPlugin::sessionLoaded()
{
    if (!evaluator)
      return;

    Core::IEditor* editor = Core::EditorManager::instance()->currentEditor();
    if (editor)
    {
      QString file_name = editor->document()->fileName();
      if (file_name.size())
        evaluator->setWorkingDir( QFileInfo( file_name ).dir().absolutePath() );
    }
}


// JuliaEditorFactory *******

JuliaEditorFactory::JuliaEditorFactory(JuliaEditorPlugin *plugin_)
  : plugin(plugin_)
{
  mime_types << QLatin1String(Constants::JULIA_MIMETYPE)
             << QLatin1String(Constants::JULIA_PROJECT_MIMETYPE);
}

Core::IEditor* JuliaEditorFactory::createEditor(QWidget *parent)
{
  JuliaEditorWidget* editor = new JuliaEditorWidget(parent);
  editor->setRevisionsVisible(true);

  emit newEditor( editor );

  return editor->editor();
}

Core::Id JuliaEditorFactory::id() const
{
  return JuliaPlugin::Constants::JULIAEDITOR_ID;
}

QString JuliaEditorFactory::displayName() const
{
  return qApp->translate( "OpenWith::Editors", Constants::JULIAEDITOR_DISPLAY_NAME);
}


Q_EXPORT_PLUGIN2(JuliaEditor, JuliaEditorPlugin)

