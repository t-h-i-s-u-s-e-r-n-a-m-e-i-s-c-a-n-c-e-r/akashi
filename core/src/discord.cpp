//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                             //
//                                                                                  //
//    This program is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU Affero General Public License as                //
//    published by the Free Software Foundation, either version 3 of the            //
//    License, or (at your option) any later version.                               //
//                                                                                  //
//    This program is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "include/discord.h"

Discord::Discord(QObject* parent) :
    QObject(parent)
{
    if (!QUrl(ConfigManager::discordWebhookUrl()).isValid())
        qWarning("Invalid webhook URL!");
    m_nam = new QNetworkAccessManager();
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &Discord::onReplyFinished);
    m_request.setUrl(QUrl(ConfigManager::discordWebhookUrl()));
}

void Discord::onModcallWebhookRequested(const QString &f_name, const QString &f_area, const QString &f_reason, const QQueue<QString> &f_buffer)
{
    QJsonDocument l_json = constructModcallJson(f_name, f_area, f_reason);
    postJsonWebhook(l_json);

    if (ConfigManager::discordWebhookSendFile()) {
        QHttpMultiPart *l_multipart = constructLogMultipart(f_buffer);
        postMultipartWebhook(*l_multipart);
    }
}

void Discord::onBanWebhookRequested(const QString &f_ipid, const QString &f_moderator, const QString &f_duration, const QString &f_reason, const int &f_banID)
{
    QJsonDocument l_json = constructBanJson(f_ipid,f_moderator, f_duration, f_reason, f_banID);
    postJsonWebhook(l_json);
}

QJsonDocument Discord::constructModcallJson(const QString &f_name, const QString &f_area, const QString &f_reason) const
{
    QJsonObject l_json;
    QJsonArray l_array;
    QJsonObject l_object {
        {"color", "13312842"},
        {"title", f_name + " filed a modcall in " + f_area},
        {"description", f_reason}
    };
    l_array.append(l_object);
    l_json["embeds"] = l_array;
    if (!ConfigManager::discordWebhookContent().isEmpty())
        l_json["content"] = ConfigManager::discordWebhookContent();

    return QJsonDocument(l_json);
}

QJsonDocument Discord::constructBanJson(const QString &f_ipid, const QString &f_moderator, const QString &f_duration, const QString &f_reason, const int &f_banID)
{
    QJsonObject l_json;
    QJsonArray l_array;
    QJsonObject l_object {
        {"color", "13312842"},
        {"title", "Ban issued by " + f_moderator},
        {"description", "Client IPID : " + f_ipid + "\nBan ID: " + QString::number(f_banID) + "\nBan reason : " + f_reason +"\nBanned until : " +f_duration}
    };
    l_array.append(l_object);
    l_json["embeds"] = l_array;

    return QJsonDocument(l_json);
}

QHttpMultiPart* Discord::constructLogMultipart(const QQueue<QString> &f_buffer) const
{
    QHttpMultiPart* l_multipart = new QHttpMultiPart();
    QHttpPart l_file;
    l_file.setRawHeader(QByteArray("Content-Disposition"), QByteArray("form-data; name=\"file\"; filename=\"log.txt\""));
    l_file.setRawHeader(QByteArray("Content-Type"), QByteArray("plain/text"));
    QString l_log;
    for (QString log_entry : f_buffer) {
        l_log.append(log_entry + "\n");
    }
    l_file.setBody(l_log.toUtf8());
    l_multipart->append(l_file);
    return l_multipart;
}

void Discord::postJsonWebhook(const QJsonDocument &f_json)
{
    if (!QUrl(m_request.url()).isValid()) {
        qWarning("Invalid webhook URL!");
        return;
    }
    m_request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->post(m_request, f_json.toJson());
}

void Discord::postMultipartWebhook(QHttpMultiPart &f_multipart)
{
    if (!QUrl(m_request.url()).isValid()) {
        qWarning("Invalid webhook URL!");
        f_multipart.deleteLater();
        return;
    }
    m_request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + f_multipart.boundary());
    QNetworkReply* l_reply = m_nam->post(m_request, &f_multipart);
    f_multipart.setParent(l_reply);
}

void Discord::onReplyFinished(QNetworkReply *f_reply)
{
    auto l_data = f_reply->readAll();
    f_reply->deleteLater();
#ifdef DISCORD_DEBUG
    QDebug() << l_data;
#endif
}

Discord::~Discord()
{
    m_nam->deleteLater();
}
