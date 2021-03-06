#include "test_dialogs.h"
#include "app/dialogs/connect.h"
#include "models/connectionsmanager.h"
#include "app/widgets/consoletabs.h"
#include <functional>
#include <QTemporaryFile>
#include <QFileInfo>

namespace ValueEditor {
    class ViewModel;
}

void TestDialogs::testConnectionDialog()
{
    //given
    QString configTestFile = "connections.json";
    ConsoleTabs tabsWidget;
    QSharedPointer<ConnectionsManager> testManager(new ConnectionsManager(configTestFile, tabsWidget,
                                   QSharedPointer<ValueEditor::ViewModel>()));
    QTemporaryFile fake_private_key;
    QFileInfo fk(fake_private_key);
    QString fake_file_path = fk.absoluteFilePath();

    //ssh with password
    RedisClient::ConnectionConfig conf("fake_host", "fake_name");
    conf.setSshTunnelSettings("fake_ssh_host", "fake_ssh_user", "fake_ssh_pass");

    //ssh with key
    RedisClient::ConnectionConfig conf2("fake_host", "fake_name");
    conf2.setSshTunnelSettings("fake_ssh_host", "fake_ssh_user", "", 2222, fake_file_path);

    //ssh with key and pass
    RedisClient::ConnectionConfig conf3("fake_host", "fake_name");
    conf3.setSshTunnelSettings("fake_ssh_host", "fake_ssh_user", "fake_ssh_key_pass", 2222, fake_file_path);

    // ssl with auth and custom port
    RedisClient::ConnectionConfig conf4("fake_host", "fake_name", 6380);
    conf4.setParam("ssl_ca_cert_path", fake_file_path);
    conf4.setParam("ssl_private_key_path", fake_file_path);
    conf4.setParam("ssl_local_cert_path", fake_file_path);
    conf4.setParam("auth", "fake_auth");

    auto verify = [this, testManager](RedisClient::ConnectionConfig& conf) {
        ConnectionWindow window(testManager.toWeakRef());

        //when
        window.setConnectionConfig(conf);
        RedisClient::ConnectionConfig actualResult = window.getConectionConfigFromFormData();

        //then
        QCOMPARE(actualResult.getInternalParameters(), conf.getInternalParameters());
        QCOMPARE(window.isFormDataValid(), true);
    };

    qDebug() << "SSH with pass";
    verify(conf);

    qDebug() << "SSH with key";
    verify(conf2);

    qDebug() << "SSH with key and pass";
    verify(conf3);

    qDebug() << "SSL with auth and custom port";
    verify(conf4);
}
