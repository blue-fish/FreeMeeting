#ifndef CJSON_H
#define CJSON_H

#include<QJsonObject>
#include<QJsonDocument>
#include<QJsonValue>

class CJson
{
public:

    CJson( ){
        QJsonParseError jsonError;
        // 转化为 JSON 文档
        QJsonDocument doucment = QJsonDocument::fromJson("{}", &jsonError);
        // 解析未发生错误
        if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
            // JSON 文档为对象
            if (doucment.isObject()) {
                json = doucment.object();
            }
        }
    }

    CJson(const char * szbuf ){
        QJsonParseError jsonError;
        // 转化为 JSON 文档
        QJsonDocument doucment = QJsonDocument::fromJson(szbuf, &jsonError);
        // 解析未发生错误
        if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
            // JSON 文档为对象
            if (doucment.isObject()) {
                json = doucment.object();
            }
        }
    }

    int json_get_int( const char* key ){
        return json.value(key).toInt();
    }
    QString json_get_string( const char* key ){
        return json.value(key).toString();
    }
    QByteArray json_get_byte_array( const char* key ){
        return json.value(key).toString().toLatin1();
    }
    void json_add_value( const char* key , int value)
    {
        json.insert( key , value );
    }
    void json_add_value( const char* key , const char* value)
    {
        json.insert( key , value );
    }

    QByteArray json_to_string()
    {
        QJsonDocument document;
        document.setObject(json);
        return document.toJson(QJsonDocument::Compact);
    }

private:
    QJsonObject json;
};


#endif // CJSON_H
