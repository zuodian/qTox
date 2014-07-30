/*
    Copyright (C) 2013 by Maxim Biro <nurupo.contributions@gmail.com>

    This file is part of Tox Qt GUI.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the COPYING file for more details.
*/

#include "smileypack.h"
#include "settings.h"

#include <QFileInfo>
#include <QFile>
#include <QtXml>
#include <QDebug>

SmileyPack::SmileyPack()
{
    load(Settings::getInstance().getSmileyPack());
    connect(&Settings::getInstance(), &Settings::smileyPackChanged, this, &SmileyPack::onSmileyPackChanged);
}

SmileyPack& SmileyPack::getInstance()
{
    static SmileyPack smileyPack;
    return smileyPack;
}

QStringList SmileyPack::listSmileyPacks(const QString &path)
{
    QStringList smileyPacks;

    QDir dir(path);
    foreach (const QString& subdirectory, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        dir.cd(subdirectory);

        QFileInfoList entries = dir.entryInfoList(QStringList() << "emoticons.xml", QDir::Files);
        if (entries.size() > 0) // does it contain a file called emoticons.xml?
            smileyPacks << QDir(QCoreApplication::applicationDirPath()).relativeFilePath(entries[0].absoluteFilePath());

        dir.cdUp();
    }

    return smileyPacks;
}

bool SmileyPack::load(const QString& filename)
{
    // discard old data
    assignmentTable.clear();
    cache.clear();
    emoticons.clear();

    // open emoticons.xml
    QFile xmlFile(filename);
    if(!xmlFile.open(QIODevice::ReadOnly))
        return false; // cannot open file

    /* parse the cfg file
     * sample:
     * <?xml version='1.0'?>
     * <messaging-emoticon-map>
     *   <emoticon file="smile.png" >
     *       <string>:)</string>
     *       <string>:-)</string>
     *   </emoticon>
     *   <emoticon file="sad.png" >
     *       <string>:(</string>
     *       <string>:-(</string>
     *   </emoticon>
     * </messaging-emoticon-map>
     */

    QDomDocument doc;
    doc.setContent(xmlFile.readAll());

    QDomNodeList emoticonElements = doc.elementsByTagName("emoticon");
    for (int i = 0; i < emoticonElements.size(); ++i)
    {
        QString file = emoticonElements.at(i).attributes().namedItem("file").nodeValue();
        QDomElement stringElement = emoticonElements.at(i).firstChildElement("string");

        QStringList emoticonSet; // { ":)", ":-)" } etc.

        while (!stringElement.isNull())
        {
            QString emoticon = stringElement.text();
            assignmentTable.insert(emoticon, file);
            emoticonSet.push_back(emoticon);

            stringElement = stringElement.nextSibling().toElement();
        }
        emoticons.push_back(emoticonSet);
    }

    path = QFileInfo(filename).absolutePath();

    // success!
    return true;
}

QString SmileyPack::replaceEmoticons(QString msg)
{
    QRegExp exp("\\S+"); // matches words

    int index = msg.indexOf(exp);

    // if a word is key of a smiley, replace it by its corresponding image in Rich Text
    while (index >= 0)
    {
        QString key = exp.cap();
        if (assignmentTable.contains(key))
        {
            QString file = assignmentTable[key];
            if (!cache.contains(file)) {
                loadSmiley(file);
            }

            QString imgRichText = "<img src=\"data:image/png;base64," % cache[file] % "\">";

            msg.replace(index, key.length(), imgRichText);
            index += imgRichText.length() - key.length();
        }
        index = msg.indexOf(exp, index + key.length());
    }

    return msg;
}

QList<QStringList> SmileyPack::getEmoticons() const
{
    return emoticons;
}

QString SmileyPack::getRichText(const QString &key)
{
    QString file = assignmentTable[key];
    if (!cache.contains(file)) {
        loadSmiley(file);
    }

    return "<img src=\"data:image/png;base64," % cache[file] % "\">";
}

QIcon SmileyPack::getIcon(const QString &key)
{
    return QIcon(path + '/' + assignmentTable[key]);
}

void SmileyPack::loadSmiley(const QString &name)
{
    QSize size(16, 16); // TODO: adapt to text size
    QString filename = path % '/' % name;
    QImage img(filename);

    if (!img.isNull())
    {
        QImage scaledImg = img.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QByteArray scaledImgData;
        QBuffer buffer(&scaledImgData);
        scaledImg.save(&buffer, "PNG");

        cache.insert(name, scaledImgData.toBase64());
    }
}

void SmileyPack::onSmileyPackChanged()
{
    load(Settings::getInstance().getSmileyPack());
}
