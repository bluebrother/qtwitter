/***************************************************************************
 *   Copyright (C) 2008-2009 by Dominik Kapusta       <d@ayoy.net>         *
 *   Copyright (C) 2009 by Mariusz Pietrzyk       <wijet@wijet.pl>         *
 *   Copyright (C) 2009 by Anna Nowak           <wiorka@gmail.com>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of      *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this program; if not, write to                     *
 *   the Free Software Foundation, Inc.,                                   *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#include <QDesktopServices>
#include <QProcess>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QDataStream>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <urlshortener/urlshortener.h>
#include "core.h"
#include "mainwindow.h"
#include "imagedownload.h"
#include "dmdialog.h"
#include "configfile.h"
#include "twitpicengine.h"
#include "statusmodel.h"
#include "statuswidget.h"
#include "welcomedialog.h"
#include "accountsmodel.h"
#include "accountscontroller.h"
#include "ui_authdialog.h"
#include "ui_twitpicnewphoto.h"

#ifdef OAUTH
#  include <oauthwizard.h>
#endif

extern ConfigFile settings;

int Core::m_requestCount = 0;

Core::Core( MainWindow *parent ) :
    QObject( parent ),
    authDialogOpen( false ),
    waitForAccounts( false ),
    settingsOpen( false ),
    checkForNew( true ),
    twitpicUpload(0),
    accounts(0),
    accountsModel(0),
    timer(0),
    parentMainWindow( parent )
{
  connect( ImageDownload::instance(), SIGNAL(imageReadyForUrl(QString,QPixmap*)), this, SLOT(setImageForUrl(QString,QPixmap*)) );

//  twitterapi = new TwitterAPIInterface( this );
//
//#ifdef OAUTH
//  twitterapi->setConsumerKey( OAuthWizard::ConsumerKey );
//  twitterapi->setConsumerSecret( OAuthWizard::ConsumerSecret );
//#endif
//
//  connect( twitterapi, SIGNAL(newEntry(TwitterAPI::SocialNetwork,QString,Entry)), this, SLOT(addEntry(TwitterAPI::SocialNetwork,QString,Entry)) );
//  connect( twitterapi, SIGNAL(deleteEntry(TwitterAPI::SocialNetwork,QString,quint64)), this, SLOT(deleteEntry(TwitterAPI::SocialNetwork,QString,quint64)) );
//  connect( twitterapi, SIGNAL(favoriteStatus(TwitterAPI::SocialNetwork,QString,quint64,bool)), this, SLOT(setFavorited(TwitterAPI::SocialNetwork,QString,quint64,bool)) );
//  connect( twitterapi, SIGNAL(postDMDone(TwitterAPI::SocialNetwork,QString,TwitterAPI::ErrorCode)), this, SIGNAL(confirmDMSent(TwitterAPI::SocialNetwork,QString,TwitterAPI::ErrorCode)) );
//  connect( twitterapi, SIGNAL(deleteDMDone(TwitterAPI::SocialNetwork,QString,quint64,TwitterAPI::ErrorCode)), this, SLOT(deleteEntry(TwitterAPI::SocialNetwork,QString,quint64)) );
//  connect( twitterapi, SIGNAL(errorMessage(QString)), this, SIGNAL(errorMessage(QString)) );
//  connect( twitterapi, SIGNAL(unauthorized(TwitterAPI::SocialNetwork,QString,QString)), this, SLOT(slotUnauthorized(TwitterAPI::SocialNetwork,QString,QString)) );
//  connect( twitterapi, SIGNAL(unauthorized(TwitterAPI::SocialNetwork,QString,QString,QString,quint64)), this, SLOT(slotUnauthorized(TwitterAPI::SocialNetwork,QString,QString,QString,quint64)) );
//  connect( twitterapi, SIGNAL(unauthorized(TwitterAPI::SocialNetwork,QString,QString,quint64,Entry::Type)), this, SLOT(slotUnauthorized(TwitterAPI::SocialNetwork,QString,QString,quint64,Entry::Type)) );

  connect( this, SIGNAL(newRequest()), SLOT(slotNewRequest()) );
//  connect( twitterapi, SIGNAL(requestDone(TwitterAPI::SocialNetwork,QString,int)), this, SLOT(slotRequestDone(TwitterAPI::SocialNetwork,QString,int)) );

  urlShortener = new UrlShortener( this );
  connect( urlShortener, SIGNAL(shortened(QString)), this, SIGNAL(urlShortened(QString)));
  connect( urlShortener, SIGNAL(errorMessage(QString)), this, SIGNAL(errorMessage(QString)));


  connect( StatusModel::instance(), SIGNAL(openBrowser(QUrl)), this, SLOT(openBrowser(QUrl)) );
  connect( StatusModel::instance(), SIGNAL(markEverythingAsRead()), this, SLOT(markEverythingAsRead()) );

  connect( StatusModel::instance(), SIGNAL(destroy(QString,QString,quint64,Entry::Type)), this, SLOT(destroy(QString,QString,quint64,Entry::Type)) );
  connect( StatusModel::instance(), SIGNAL(favorite(QString,QString,quint64,bool)), this, SLOT(favoriteRequest(QString,QString,quint64,bool)) );
  connect( StatusModel::instance(), SIGNAL(postDM(QString,QString,QString)), this, SLOT(postDMDialog(QString,QString,QString)) );

  accountsModel = new AccountsModel;

//#if defined Q_WS_MAC || defined Q_WS_WIN
//  QFile file( QDesktopServices::storageLocation( QDesktopServices::DataLocation ) + "/qTwitter/state" );
//#else
//  QFile file( QDesktopServices::storageLocation( QDesktopServices::HomeLocation ) + "/.qtwitter/state" );
//#endif
//  bool ok = file.open(QIODevice::ReadOnly);
//  if (ok) {
//    QDataStream in(&file);
//    int count;
//    in >> count;
////    qDebug() << "Stream status:" << in.status();
//    if ( in.status() == QDataStream::Ok ) {
////      qDebug() << __FUNCTION__ << "Accounts count:" << count;
//
//      QList<Account> accountsList;
//
//      for( int i = 0; i < count; ++i ) {
//        Account *account = new Account;
//        in >> *account;
//        accountsList << *account;
//        statusLists.insert( account, new StatusList( account.login, account.network, this ) );
//      }
//
////      qDebug() << "Stream status:" << in.status();
//      if ( in.status() == QDataStream::Ok ) {
//        accountsModel->setAccounts( accountsList );
//
//        foreach( StatusList *statusList, statusLists.values() ) {
//          qint8 visible;
//          qint8 active;
//          QList<Status> list;
//          in >> visible;
//          in >> active;
//          in >> list;
//          statusList->setStatuses( list );
//          statusList->setVisible( (bool) visible );
//          statusList->setActive( active );
//        }
//      }
////      qDebug() << "Stream status:" << in.status();
//    }
//  }
//  file.close();
}

Core::~Core()
{
//#if defined Q_WS_MAC || defined Q_WS_WIN
//  QString path( QDesktopServices::storageLocation( QDesktopServices::DataLocation ) + "/qTwitter" );
//#else
//  QString path( QDesktopServices::storageLocation( QDesktopServices::HomeLocation ) + "/.qtwitter" );
//#endif
//  QDir dir( path );
//  if ( !dir.exists() && !dir.mkpath( path ) ) {
//    return;
//  }
//  QFile file( path + "/state" );
//  bool ok = file.open(QIODevice::WriteOnly);
//  if (ok) {
//    QDataStream out(&file);
//    QList<Account*> list = statusLists.keys();
//    out << list.size();
//    foreach ( Account *account, list ) {
//      out << *account;
//    }
//    foreach ( StatusList *statusList, statusLists.values() ) {
//      out << (qint8) statusList->isVisible();
//      out << (qint8) statusList->active();
//      out << statusList->getData();
//    }
//    file.close();
//  }
}

void Core::createAccounts( QWidget *view )
{
  accounts = new AccountsController( view, this );
  connect( accounts, SIGNAL(comboActive(bool)), this, SLOT(setWaitForAccounts(bool)) );
  connect( accounts, SIGNAL(accountDialogClosed(bool)), this, SIGNAL(accountDialogClosed(bool)) );
  if ( accountsModel )
    accounts->setModel( accountsModel );
  else
    qWarning() << "Accounts model not present!";
}

void Core::setWaitForAccounts( bool wait )
{
  if ( !settingsOpen && waitForAccounts && !wait ) {
    waitForAccounts = wait;
    applySettings();
    if ( accounts->isModified() ) {
      get();
    }
    return;
  }
  waitForAccounts = wait;
}

void Core::markEverythingAsRead()
{
  foreach ( StatusList *statusList, statusLists.values() ) {
    statusList->markAllAsRead();
  }
}

void Core::setSettingsOpen( bool open )
{
  settingsOpen = open;
  if ( open ) {
    accounts->setModified( false );
  }
}

QStringList Core::twitpicLogins() const
{
  QStringList list;
  foreach ( Account account, accountsModel->getAccounts() ) {
    if ( account.serviceUrl() == Account::NetworkUrlTwitter ) {
      list << account.login();
    }
  }
  return list;
}

void Core::applySettings()
{
  if ( settings.contains( "FIRSTRUN" ) || settings.value( "OAuth", true ) == false ) {
    QString val = settings.value( "FIRSTRUN" ).toString();
    if ( val == "ever" ) {
      WelcomeDialog dlg;
      connect( &dlg, SIGNAL(addAccount()), this, SLOT(addAccount()) );
      connect( this, SIGNAL(accountDialogClosed(bool)), &dlg, SLOT(confirmAccountAdded(bool)) );
      dlg.exec();
    }
#ifdef OAUTH
    else if ( val == ConfigFile::APP_VERSION || settings.value( "OAuth", true ) == false ) {
      settings.removeOldTwitterAccounts();
      QMessageBox dlg;
      dlg.setWindowTitle( tr( "Welcome!" ) );
      //: e.g. "Welcome to qTwitter 0.8.1!"
      dlg.setText( tr( "Welcome to qTwitter %1!" ).arg( ConfigFile::APP_VERSION ) );
      dlg.setInformativeText( tr( "<b>Please note:</b> we introduced the ultra-secure OAuth "
                                  "authorization scheme for Twitter accounts in this release. This "
                                  "means that you would never be asked again to enter your Twitter "
                                  "password in qTwitter. However, your existing qTwitter "
                                  "configuration will not work any more. Your Twitter accounts and "
                                  "passwords have been removed from qTwitter settings, please "
                                  "reconfigure them in Settings to have things working. "
                                  "Thanks and enjoy qTwitter!" ) );
      dlg.setIconPixmap( QPixmap( ":icons/twitter_48.png" ) );
      dlg.exec();
      settings.setValue( "OAuth", true );
    }
#endif
    settings.remove( "FIRSTRUN" );
  }

  static bool appStartup = true;
  if ( appStartup ) {
    accounts->loadAccounts();
    appStartup = false;
  }

  if ( !waitForAccounts ) {

    int mtc = settings.value( "Appearance/tweet count", 20 ).toInt();
    StatusList::setMaxCount( mtc );
    StatusModel::instance()->setMaxStatusCount( mtc );
    setTimerInterval( settings.value( "General/refresh-value", 15 ).toInt() * 60000 );

    setupStatusLists();
    if ( statusLists.count() == 0 ) {
      StatusModel::instance()->clear();
    }

    emit accountsUpdated( accountsModel->getAccounts() );

    // TODO: do this only when really needed
//    twitterapi->resetConnections();

    emit resetUi();
    m_requestCount = 0;

    if ( accounts->isModified() ) {
      get();
    }
  }
}

bool Core::setTimerInterval( int msecs )
{
  bool initialization = !(bool) timer;
  if ( initialization ) {
    timer = new QTimer( this );
    connect( timer, SIGNAL(timeout()), this, SLOT(get()) );
  }
  if ( timer->interval() != msecs ) {
    timer->setInterval( msecs );
    timer->start();
    if ( !initialization ) {
      return true;
    }
  }
  return false;
}

#ifdef Q_WS_X11
void Core::setBrowserPath( const QString &path )
{
  browserPath = path;
}
#endif

void Core::setModelTheme( const ThemeData &theme )
{
  StatusModel::instance()->setTheme( theme );
}

void Core::setModelData( const QString &serviceUrl, const QString &login )
{
  //TODO: debug, warning, etc.
  if ( login.isNull() ) {
    StatusModel::instance()->clear();
  } else {
    Account *accountToShow = accountsModel->account( serviceUrl, login );
    foreach( Account *account, statusLists.keys() ) {
      if ( *account == *accountToShow ) {
        StatusModel::instance()->setStatusList( statusLists[ account ] );
        break;
      }
    }
  }
}

void Core::forceGet()
{
  timer->start();
  get();
}

void Core::get( const QString &serviceUrl, const QString &login, const QString &password )
{
  // TODO: remove
  Q_UNUSED(password);

  Account *account = accountsModel->account( serviceUrl, login );
  if ( account ) {
    StatusList *statusList = statusLists.value( account );
    if ( statusList ) {
      statusList->requestFriendsTimeline();
      // TODO: newRequest from StatusList or TwitterAPI
      emit newRequest();
      if ( statusList->dm() ) {
        statusList->requestDirectMessages();
        emit newRequest();
      }
    }
  }
}

void Core::get()
{
  bool started = false;
  foreach( StatusList *statusList, statusLists.values() ) {
    // TODO: WTF?
    started = true;
    statusList->requestFriendsTimeline();
    emit newRequest();
    if ( statusList->dm() ) {
      statusList->requestDirectMessages();
      emit newRequest();
    }
  }

  if ( started ) {
    emit requestStarted();
  }
}

void Core::post( const QString &serviceUrl, const QString &login, const QString &status, quint64 inReplyTo )
{
  Account *account = accountsModel->account( serviceUrl, login );
  if ( account ) {
    StatusList *statusList = statusLists.value( account );
    if ( statusList ) {
      statusList->requestNewStatus( status, inReplyTo );
      emit newRequest();
    }
  }
  emit requestStarted();
  // TODO: WTF?
  checkForNew = false;
}

void Core::destroy( const QString &serviceUrl, const QString &login, quint64 id, Entry::Type type )
{
  Account *account = accountsModel->account( serviceUrl, login );
  if ( account ) {
    StatusList *statusList = statusLists.value( account );
    if ( statusList ) {
      if ( settings.value( "General/confirmTweetDeletion", true ).toBool() ) {
        QMessageBox *confirm = new QMessageBox( QMessageBox::Warning,
                                                //: Are you sure to delete your message
                                                tr( "Are you sure?" ),
                                                tr( "Are you sure to delete this status?" ),
                                                QMessageBox::Yes | QMessageBox::Cancel,
                                                parentMainWindow );
        int result = confirm->exec();
        delete confirm;
        if ( result == QMessageBox::Cancel )
          return;
      }
      statusList->requestDestroy( id, type );
      emit newRequest();
      emit requestStarted();
      checkForNew = false;
    }
  }
}

//void Core::destroyDontAsk( const QString &serviceUrl, const QString &login, quint64 id, Entry::Type type )
//{
//  if ( type == Entry::Status ) {
//    twitterapi->deleteUpdate( network, login, accountsModel->account( network, login )->password, id );
//  } else if ( type == Entry::DirectMessage ) {
//    twitterapi->deleteDM( network, login, accountsModel->account( network, login )->password, id );
//  }
//  emit newRequest();
//  emit requestStarted();
//  checkForNew = false;
//}

void Core::favoriteRequest( const QString &serviceUrl, const QString &login, quint64 id, bool favorited )
{
  Account *account = accountsModel->account( serviceUrl, login );
  if ( account ) {
    StatusList *statusList = statusLists.value( account );
    if ( statusList ) {
      if ( favorited ) {
        statusList->requestCreateFavorite( id );
      } else {
        statusList->requestDestroyFavorite( id );
      }
      checkForNew = false;
      emit newRequest();
      emit requestStarted();
    }
  }
}

void Core::postDMDialog( const QString &serviceUrl, const QString &login, const QString &screenName )
{
  DMDialog *dlg = new DMDialog( serviceUrl, login, screenName, parentMainWindow );
  connect( dlg, SIGNAL(dmRequest(QString,QString,QString,QString)), this, SLOT(postDM(QString,QString,QString,QString)) );
  connect( this, SIGNAL(confirmDMSent(QString,QString,TwitterAPI::ErrorCode)), dlg, SLOT(showResult(QString,QString,TwitterAPI::ErrorCode)) );

  dlg->exec();
  dlg->deleteLater();
}

void Core::postDM( const QString &serviceUrl, const QString &login, const QString &screenName, const QString &text )
{
  Account *account = accountsModel->account( serviceUrl, login );
  if ( account ) {
    StatusList *statusList = statusLists.value( account );
    if ( statusList ) {
      statusList->requestNewDM( screenName, text );
    }
  }
}

void Core::uploadPhoto( const QString &login, QString photoPath, QString status )
{
  bool ok = false;
  QString password = QInputDialog::getText( 0, tr( "Enter password" ), tr( "Enter your Twitter password.<br>We're not storing it anywhere" ),
                                            QLineEdit::Password, QString(), &ok );
  if ( ok && !password.isEmpty() ) {
    twitpicUpload = new TwitPicEngine( this );
    qDebug() << "uploading photo";
    twitpicUpload->postContent( login, password, photoPath, status );
  }
}

void Core::abortUploadPhoto()
{
  if ( twitpicUpload ) {
    twitpicUpload->abort();
    twitpicUpload->deleteLater();
    twitpicUpload = 0;
  }
}

void Core::twitPicResponse( bool responseStatus, QString message, bool newStatus )
{
  qDebug() << "twicPicResponse";
  emit twitPicResponseReceived();
  if ( !responseStatus ) {
    emit errorMessage( tr( "There was a problem uploading your photo:" ).append( " %1" ).arg( message ) );
    return;
  }
  if ( newStatus ) {
    forceGet();
  }
  twitpicUpload->deleteLater();
  twitpicUpload = 0;
  QDialog dlg;
  Ui::TwitPicNewPhoto ui;
  ui.setupUi( &dlg );
  ui.textBrowser->setText( tr( "Photo available at:" ).append( " <a href=\"%1\">%1</a>" ).arg( message ) );
  dlg.exec();
}

void Core::openBrowser( QUrl address )
{
  if ( address.isEmpty() )
    return;
#if defined Q_WS_MAC || defined Q_WS_WIN
  QDesktopServices::openUrl( address );
#elif defined Q_WS_X11
  QProcess *browser = new QProcess;
  if ( browserPath.isNull() ) {
    QDesktopServices::openUrl( address );
    return;
  }
  browser->start( browserPath + " " + address.toString() );
#endif
}

Core::AuthDialogState Core::authDataDialog( Account *account )
{
  if ( authDialogOpen )
    return Core::STATE_DIALOG_OPEN;
  emit pauseIcon();
  QDialog *dlg = new QDialog( parentMainWindow );
  Ui::AuthDialog ui;
  ui.setupUi( dlg );
  //: This is for newly created account - when the login isn't given yet
  ui.loginEdit->setText( ( account->login() == tr( "<empty>" ) ) ? QString() : account->login() );
  ui.loginEdit->selectAll();
  ui.passwordEdit->setText( account->password() );
  dlg->adjustSize();
  authDialogOpen = true;
  int row = accountsModel->indexOf( *account );
  if ( dlg->exec() == QDialog::Accepted ) {
    if ( ui.disableBox->isChecked() ) {
      authDialogOpen = false;
      account->setEnabled( false );
      settings.setValue( QString("Accounts/%1/enabled").arg( row ), false );
      emit accountsUpdated( accountsModel->getAccounts() );
      delete dlg;
      return Core::STATE_DISABLE_ACCOUNT;
    } else if ( ui.removeBox->isChecked() ) {
      authDialogOpen = false;
      accountsModel->removeRow( row );
      settings.deleteAccount( row, accountsModel->rowCount() );
      emit accountsUpdated( accountsModel->getAccounts() );
      delete dlg;
      return Core::STATE_REMOVE_ACCOUNT;
    }
//    if ( account->login() != ui.loginEdit->text() ) {
//      Account newAccount = *account;
//      newAccount.setLogin( ui.loginEdit->text() );
//      statusLists[ newAccount ] = statusLists[ *account ];
//      statusLists[ newAccount ]->setLogin( newAccount.login() );
//      statusLists.remove( *account );
//      account->setLogin( ui.loginEdit->text() );
//      settings.setValue( QString("Accounts/%1/login").arg( accountsModel->indexOf( *account ) ), account->login() );
//      emit accountsUpdated( accountsModel->getAccounts() );
//    }
    account->setLogin( ui.loginEdit->text() );
    account->setPassword( ui.passwordEdit->text() );

    settings.setValue( QString("Accounts/%1/login").arg( accountsModel->indexOf( *account ) ), account->login() );

    if ( settings.value( "General/savePasswords", Qt::Unchecked ).toInt() == Qt::Checked )
      settings.setValue( QString("Accounts/%1/password").arg( accountsModel->indexOf( *account ) ), ConfigFile::pwHash( account->password() ) );

    authDialogOpen = false;
    emit requestStarted();
    delete dlg;
    return Core::STATE_ACCEPTED;
  }
  authDialogOpen = false;
  delete dlg;
  return Core::STATE_REJECTED;
}

void Core::retranslateUi()
{
  StatusModel::instance()->retranslateUi();
    if ( accounts )
      accounts->retranslateUi();
}


void Core::setImageForUrl( const QString& url, QPixmap *image )
{
  Status status;
  foreach ( StatusList *statusList, statusLists )
  {
    for ( int i = 0; i < statusList->size(); i++ ) {
      status = statusList->data(i);
      if ( status.entry.type == Entry::Status && url == status.entry.userInfo.imageUrl ) {
        statusList->setImage( i, *image );
      }
    }
  }
}

void Core::setupStatusLists()
{
  accountsModel->cleanUp();
  QList<Account> modelAccounts = accountsModel->getAccounts(); // get accounts from model

  // delete status lists for accounts removed from model
  foreach( Account *account, statusLists.keys() ) {
    if ( !modelAccounts.contains( *account ) ) {
      statusLists[ account ]->deleteLater();
      delete account;
      qDebug() << "Deleting statusList for account:" << account->login() << account->serviceUrl();
      statusLists.remove( account );
    }
  }

  // check modelAccounts one by one
  foreach ( Account modelAccount, modelAccounts ) {
    if ( modelAccount.isEnabled() ) {    // if it's enabled
      bool contains = false;             // check if a status list for it exists
      foreach( Account *account, statusLists.keys() ) {
        if ( *account == modelAccount ) {
          contains = true;
          break;
        }
      }
      if ( !contains ) {     // if the status list doesn't exists
        Account *newAccount = new Account( modelAccount ); // create one
        StatusList *newStatusList = new StatusList( newAccount, this );
        statusLists.insert( newAccount, newStatusList ); // and add it
      }
    } else {   // if account is not enabled
      Account *accountToRemove = 0; // check if a status list for it exists, and store result in a pointer
      foreach( Account *account, statusLists.keys() ) {
        if ( *account == modelAccount ) {
          accountToRemove = account;
          break;
        }
      }
      if ( accountToRemove ) {  // if a status list exist for a removed account
        statusLists[ accountToRemove ]->deleteLater(); // remove it
        statusLists.remove( accountToRemove );
        delete accountToRemove; // and delete the account
      }
    }
  }

  foreach ( Account *account, statusLists.keys() ) {
    statusLists[ account ]->slotDirectMessagesChanged( account->dm() );
  }

  m_requestCount = 0;
}

bool Core::retryAuthorizing( Account *account, int role )
{
  if ( !account )
    return false;

  Core::AuthDialogState state = authDataDialog( account );
  switch ( state ) {
  case Core::STATE_ACCEPTED:
    return true;
  case Core::STATE_REJECTED:
    switch ( role ) {
    case TwitterAPI::ROLE_POST_UPDATE:
      emit errorMessage( tr( "Authentication is required to post updates." ) );
      break;
    case TwitterAPI::ROLE_POST_DM:
      emit errorMessage( tr( "Authentication is required to send direct messages." ) );
      break;
    case TwitterAPI::ROLE_DELETE_UPDATE:
      emit errorMessage( tr( "Authentication is required to delete updates." ) );
      break;
    case TwitterAPI::ROLE_FRIENDS_TIMELINE:
      emit errorMessage( tr( "Authentication is required to get your friends' updates." ) );
      break;
    case TwitterAPI::ROLE_DIRECT_MESSAGES:
      break;
    case TwitterAPI::ROLE_PUBLIC_TIMELINE:
      break;
    }
  case Core::STATE_DIALOG_OPEN:
  case Core::STATE_REMOVE_ACCOUNT:
  case Core::STATE_DISABLE_ACCOUNT:
  default:;
  }
  return false;
}

void Core::slotNewRequest()
{
  m_requestCount++;
  qDebug() << m_requestCount << "request(s) pending";
}

void Core::resetRequestsCount()
{
  if ( m_requestCount > 0 ) {
    m_requestCount = 0;
//    QMessageBox::warning( parentMainWindow, tr( "Warning" ),
//                          tr( "One or more requests didn't complete. "
//                              "Check your connection and/or accounts settings." ),
//                          QMessageBox::Ok );
    qDebug() << "warning: some requests may have failed...";
  }
}

void Core::addAccount()
{
  accounts->addAccount();
}

void Core::slotRequestDone( const QString &serviceUrl, const QString &login, int role )
{
  StatusList *statusList = StatusModel::instance()->getStatusList();
  if ( statusList->serviceUrl() == serviceUrl
       && statusList->login() == login ){
    StatusModel::instance()->updateDisplay();
  }
  if ( role != TwitterAPI::ROLE_POST_DM && m_requestCount > 0 ) {
    m_requestCount--;
  }
  qDebug() << m_requestCount;
  if ( m_requestCount == 0 ) {
    if ( checkForNew )
      checkUnreadStatuses();
    emit resetUi();
  }
  checkForNew = true;
}

void Core::checkUnreadStatuses()
{
  QStringList unread;
  QString message;
  foreach ( Account *account, statusLists.keys() ) {
    if ( statusLists[ account ]->hasUnread() ) {
      unread.append( QString( "%1 @%2" ).arg( account->login(), Account::networkName( account->serviceUrl() ) ) );
    }
  }

  if ( unread.isEmpty() )
    return;

  if ( unread.count() == 1 ) {
    message.append( unread.at(0) );
    //: "For <user_name>"
    emit sendNewsReport( tr( "For %1" ).arg(message) );
  } else {
    message.append( unread.join( "\n" ) );
    //: There goes "For", a colon, a new line, and a list of users that heave unread statuses.
    emit sendNewsReport( tr( "For:\n%1" ).arg(message) );
  }
}

void Core::shortenUrl( const QString &url )
{
  urlShortener->shorten(url, (UrlShortener::Shortener) settings.value( "General/url-shortener" ).toInt() );
}
