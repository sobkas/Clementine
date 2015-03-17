/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>
   Copyright 2014, Krzysztof Sobiecki <sobkas@gmail.com>
   Copyright 2014, John Maguire <john.maguire@gmail.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INTERNET_PODCASTS_PODCASTPARSER_H_
#define INTERNET_PODCASTS_PODCASTPARSER_H_

#include <QStringList>
#include <QObject>
#include <tinyxml2.h>

#include "podcast.h"
#include "podcasturlloader.h"

class QXmlStreamReader;

// Reads XML data from a QByteArray.
// Returns PodcastList.
class PodcastParser : public QObject {
  Q_OBJECT

 public:
  PodcastParser();

  void close_dialog();
  static const char* kAtomNamespace;
  static const char* kItunesNamespace;

  const QStringList& supported_mime_types() const {
    return supported_mime_types_;
  }
  bool SupportsContentType(const QString& content_type) const;

  // Returns PodcastList, always!
  PodcastList Load(QByteArray data, const QUrl& url, bool verbose) const;

  // Is it still needed?
  bool TryMagic(const QByteArray& data) const;
  virtual void fool() {};
  bool loop_ = true;

 private:
  bool ParseRss(tinyxml2::XMLDocument* doc, Podcast* ret, const QUrl& url, bool verbose) const;
  void ParseItem(tinyxml2::XMLElement* entryElement, Podcast* ret) const;

  bool ParseOpml(tinyxml2::XMLDocument* doc, PodcastList* ret) const;
  bool ParseFeed(tinyxml2::XMLDocument* doc, Podcast* ret, const QUrl& url, bool verbose) const;
  void ParseEntry(tinyxml2::XMLElement* entryElement, Podcast* ret) const;
  QString findElement(tinyxml2::XMLElement* searchedElement, QStringList names, QString tag) const;
  QString findElement(tinyxml2::XMLElement* searchedElement, QString namea, QString namfb, QString tag) const;
  QString findElement(tinyxml2::XMLElement* searchedElement, QStringList names, QString attribute, QString tag) const;
  QUrl findUrl(tinyxml2::XMLElement* searchedElement, QStringList names, QString tag) const;
  QUrl findUrl(tinyxml2::XMLElement* searchedElement, QStringList names, QString attribute, QString tag) const;
  qint64 parseDuration(QString duration) const;

 private:
  QStringList supported_mime_types_;
};

#endif  // INTERNET_PODCASTS_PODCASTPARSER_H_
