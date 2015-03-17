/* This file is part of Clementine.
   Copyright 2012, 2014, David Sansome <me@davidsansome.com>
   Copyright 2014, John Maguire <john.maguire@gmail.com>
   Copyright 2014, Krzysztof Sobiecki <sobkas@gmail.com>

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

#include "podcastparser.h"

#include <QDateTime>
#include <QXmlStreamReader>
#include <tinyxml2.h>

#include "core/logging.h"
#include "core/utilities.h"

// Namespace constants must be lower case.
const char* PodcastParser::kAtomNamespace = "http://www.w3.org/2005/atom";
const char* PodcastParser::kItunesNamespace = "http://www.itunes.com/dtds/podcast-1.0.dtd";

PodcastParser::PodcastParser() {
  qLog(Debug) << "PodcastParser() was run";
  supported_mime_types_ << "application/rss+xml"
                        << "application/xml"
                        << "text/x-opml"
                        << "text/xml"
                        << "application/atom+xml";
}

bool PodcastParser::SupportsContentType(const QString& content_type) const {
  if (content_type.isEmpty()) {
    // Why not have a go.
    return true;
  }

  for (const QString& mime_type : supported_mime_types()) {
    if (content_type.contains(mime_type)) {
      return true;
    }
  }
  return false;
}

bool PodcastParser::TryMagic(const QByteArray& data) const {
  QString str(QString::fromUtf8(data));
  return str.contains(QRegExp("<rss\\b")) || str.contains(QRegExp("<feed\\b")) || str.contains(QRegExp("<opml\\b"));
}

PodcastList PodcastParser::Load(QByteArray data, const QUrl& url, bool verbose) const {
  tinyxml2::XMLDocument doc;
  doc.Parse(data.data());
  if (!doc.RootElement()) {
    return PodcastList();
  }
  
  const QString name(doc.RootElement()->Name());
  qLog(Debug) << "Name of the reader:" << name << "on url" <<url;
  if (name == "rss") {
    Podcast podcast;
    if (!ParseRss(&doc, &podcast, url, verbose)) {
      return PodcastList();
    } else {
      return PodcastList() << podcast;
    }
  } else if (name == "opml") {
    PodcastList listofpodcasts;
    if (!ParseOpml(&doc, &listofpodcasts)) {
      return PodcastList();
    } else {
      return listofpodcasts;
    }
  } else if (name == "feed") {
    Podcast podcast;
    if (!ParseFeed(&doc, &podcast, url, verbose)) {
      return PodcastList();
    } else {
      return PodcastList() << podcast;
    }
  }
  return PodcastList();
}

bool PodcastParser::ParseRss(tinyxml2::XMLDocument* doc, Podcast* ret, const QUrl& url, bool verbose) const {
  tinyxml2::XMLElement* feedElement = doc->RootElement()->FirstChildElement("channel");
  if (!feedElement) {
    return false;
  }
  ret->set_title(findElement(feedElement, QStringList() << "title", "title"));
  ret->set_link(QUrl(findElement(feedElement, QStringList() << "link", "link")));
  ret->set_url(QUrl(findElement(feedElement, QStringList() << "itunes:new-feed-url", "url")));
  if (ret->url().isEmpty()) {
    ret->set_url(url);
  }
 
  ret->set_description(findElement(feedElement, QStringList() << "description", "description"));
  ret->set_copyright(findElement(feedElement, QStringList() << "copyright", "copyright"));
  ret->set_image_url_large(QUrl(findElement(feedElement, "image", "url", "image")));
  ret->set_owner_name(findElement(feedElement, "itunes:owner", "itunes:name", "owner:name"));
  if (ret->owner_name().isEmpty()) {
    ret->set_owner_name(findElement(feedElement, QStringList() << "itunes:author", "owner:author"));
  }
  if (ret->owner_name().isEmpty()) {
    ret->set_owner_name(ret->title());
  }
  ret->set_owner_email(findElement(feedElement, "itunes:owner", "itunes:email", "owner:email"));
  tinyxml2::XMLElement* itemElement = feedElement->FirstChildElement("item");
  while (itemElement && verbose) {
    ParseItem(itemElement, ret);
    itemElement = itemElement->NextSiblingElement("item");
  }
  return true;
}

void PodcastParser::ParseItem(tinyxml2::XMLElement* itemElement, Podcast* ret) const {
  PodcastEpisode episode;
  episode.set_title(findElement(itemElement, QStringList() << "title", "item:title"));

  episode.set_description(findElement(itemElement, QStringList() << "description", "item:description"));
  QString date(findElement(itemElement, QStringList() << "pubDate", "item:pubdate"));
  episode.set_publication_date(Utilities::ParseRFC822DateTime(date));

  if (!episode.publication_date().isValid()) {
    qLog(Error) << "Unable to parse date:" << date
                << "Please submit it to "
                << "https://github.com/clementine-player/Clementine/issues/new?title="
                 + QUrl::toPercentEncoding(QString("[podcast] Unable to parse date: %1").arg(date));
    episode.set_publication_date(QDateTime::currentDateTime());
  }

  episode.set_author(findElement(itemElement, QStringList() << "itunes:author" << "author", "item:author"));
  if (episode.author().isEmpty()) {
    episode.set_author(ret->owner_name());
  }

  episode.set_url(findUrl(itemElement, QStringList() << "enclosure", "url", "item:url"));
  qLog(Debug) << "Episodes url:" << episode.url();
  if (episode.url().isEmpty()) {
    return;
  }

  QString duration(findElement(itemElement, QStringList() << "itunes:duration", "item:duration"));
  qint64 duration_episode = parseDuration(duration);
  qLog(Debug) << "Duratio of an episode:" << duration_episode << "from:" <<duration;
  episode.set_duration_secs(duration_episode);
  ret->add_episode(episode);
}

bool PodcastParser::ParseOpml(tinyxml2::XMLDocument* doc, PodcastList* listofpodcasts) const {
  tinyxml2::XMLElement* feedElement = doc->RootElement()->FirstChildElement( "body" );
  tinyxml2::XMLElement* entryElement = feedElement->FirstChildElement("outline");
  QList<tinyxml2::XMLElement*> XMLElementlist;
  qLog(Debug) << "F Check State of continues:" << loop_;
  while (entryElement && loop_) {
    qLog(Debug) << "FF Check State of continues:" << loop_;
    if (!entryElement->Attribute("type")) {
      XMLElementlist.append(entryElement);
      entryElement = entryElement->FirstChildElement("outline");
      if (!entryElement && !XMLElementlist.isEmpty()) {
        entryElement = XMLElementlist.last();
        XMLElementlist.pop_back();
        entryElement = entryElement->NextSiblingElement("outline");
      }
      
    } else {
      Podcast podcast;
      if(entryElement->Attribute("url")) {
        QUrl url_tmp(entryElement->Attribute("url"));
        qLog(Debug) << "Will process rss url:" << url_tmp;
        QByteArray data_tmp = Utilities::GetData(url_tmp);
        #if 0
        if (!data_tmp.isEmpty()) {
          listofpodcasts->append(Load(data_tmp, url_tmp, false));
        }
        #endif
      }
      if(entryElement->Attribute("xmlUrl")) {
        QUrl url_tmp(entryElement->Attribute("xmlUrl"));
        qLog(Debug) << "Will process rss url:" << url_tmp;
        QByteArray data_tmp = Utilities::GetData(url_tmp);
        #if 0
        if (!data_tmp.isEmpty()) {
          listofpodcasts->append(Load(data_tmp, url_tmp, false));
        }
        #endif
      }
      entryElement = entryElement->NextSiblingElement("outline");
      if (!entryElement && !XMLElementlist.isEmpty()) {
        entryElement = XMLElementlist.last();
        XMLElementlist.pop_back();
        entryElement = entryElement->NextSiblingElement("outline");
  
      }
    }
    qLog(Debug) << "Next loop";
    qLog(Debug) << "Check State of continues:" << loop_;
    if (!loop_) {
      return false;
    }
  }
  return true;
}

bool PodcastParser::ParseFeed(tinyxml2::XMLDocument* doc, Podcast* ret, const QUrl& url, bool verbose) const {
  tinyxml2::XMLElement* feedElement = doc->FirstChildElement("feed");
  if (!feedElement) {
    return false;
  }
  ret->set_title(findElement(feedElement, QStringList() << "title", "title"));
  ret->set_link(QUrl(findElement(feedElement, QStringList() << "link", "href", "link:href")));
  ret->set_url(QUrl(findElement(feedElement, QStringList() << "id", "id")));
  if (ret->url().isEmpty()) {
    ret->set_url(url);
  }
  tinyxml2::XMLElement* entryElement = feedElement->FirstChildElement("entry");
  while (entryElement && verbose) {
    ParseEntry(entryElement, ret);
    entryElement = entryElement->NextSiblingElement();
  }
  return true;
}

void PodcastParser::ParseEntry(tinyxml2::XMLElement* entryElement, Podcast* ret) const {
  PodcastEpisode episode;
  episode.set_title(findElement(entryElement, QStringList() << "title", "title"));
  episode.set_description(findElement(entryElement, QStringList() << "summary", "summary"));
  QString date(findElement(entryElement, QStringList() << "published", "published"));
  episode.set_publication_date(Utilities::ParseISO8601DateTime(date));

  if (!episode.publication_date().isValid()) {
    qLog(Error) << "Unable to parse date:" << date
                << "Please submit it to "
                << "https://github.com/clementine-player/Clementine/issues/new?title="
                 + QUrl::toPercentEncoding(QString("[podcast] Unable to parse date: %1").arg(date));
    episode.set_publication_date(QDateTime::currentDateTime());
  }

  episode.set_author(findElement(entryElement, "author", "name", "author:name"));

  tinyxml2::XMLElement* linkElement = entryElement->FirstChildElement("link");
  while (linkElement) {
    if (QString(linkElement->Attribute("type")).contains("audio/")) {
      episode.set_url(QUrl::fromEncoded(linkElement->Attribute("href")));
      break;
    }
    linkElement = linkElement->NextSiblingElement("link");
  }
  ret->add_episode(episode);
}

QString PodcastParser::findElement(tinyxml2::XMLElement* searchedElement, QStringList names, QString tag) const {
  for (QString name : names) {
    tinyxml2::XMLElement* tmpElement = searchedElement->FirstChildElement(name.toStdString().c_str());
    if (tmpElement) {
      QString ret(QString().fromUtf8(tmpElement->GetText()));
      return ret;
    }
  }
  qLog(Error) << "Setting tag " << tag << " failed";
  return QString();
}

QString PodcastParser::findElement(tinyxml2::XMLElement* searchedElement, QString namea, QString nameb, QString tag) const {
    tinyxml2::XMLElement* tmpElement = searchedElement->FirstChildElement(namea.toStdString().c_str());
    if (tmpElement) {
      tmpElement = tmpElement->FirstChildElement(nameb.toStdString().c_str());
        if (tmpElement) {
          QString ret(QString().fromUtf8(tmpElement->GetText()));
          return ret;
        }
    }
  qLog(Error) << "Setting tag " << tag << " failed";
  return QString();
}

QString PodcastParser::findElement(tinyxml2::XMLElement* searchedElement, QStringList names, QString attribute, QString tag) const {
  for (QString name : names) {
    tinyxml2::XMLElement* tmpElement = searchedElement->FirstChildElement(name.toStdString().c_str());
    if (tmpElement) {
      QString ret(QString().fromUtf8(tmpElement->Attribute(attribute.toStdString().c_str())));
      return ret;
    }
  }
  qLog(Error) << "Setting tag " << tag << " failed";
  return QString();
}

QUrl PodcastParser::findUrl(tinyxml2::XMLElement* searchedElement, QStringList names, QString tag) const {
  for (QString name : names) {
    tinyxml2::XMLElement* tmpElement = searchedElement->FirstChildElement(name.toStdString().c_str());
    if (tmpElement) {
      QByteArray ret(tmpElement->GetText());
      return QUrl::fromEncoded(ret);
    }
  }
  qLog(Error) << "Setting tag " << tag << " failed";
  return QUrl();
}

QUrl PodcastParser::findUrl(tinyxml2::XMLElement* searchedElement, QStringList names, QString attribute, QString tag) const {
  for (QString name : names) {
    tinyxml2::XMLElement* tmpElement = searchedElement->FirstChildElement(name.toStdString().c_str());
    if (tmpElement) {
      QByteArray ret(tmpElement->Attribute(attribute.toStdString().c_str()));
      return QUrl::fromEncoded(ret);
    }
  }
  qLog(Error) << "Setting tag " << tag << " failed";
  return QUrl();
}

qint64 PodcastParser::parseDuration(QString duration) const {
  qLog(Debug) << "Duration to be parsed: " << duration;
  if (!duration.contains(':')) {
    return duration.toInt();
  }
  QStringList durations = duration.split(':');
  QList<int> tab;
  tab << (24*3600) << 3600 << 60 << 1;
  if (durations.count() > tab.count()) {
    return qint64();
  }
  int nums = tab.count()-1;
  qint64 ret = 0;
  for (int num = durations.count()-1; num >= 0; --num) {
    ret += durations[num].toInt()*tab[nums];
    --nums;
  }
  return ret;
}
void PodcastParser::close_dialog() {
  qLog(Debug)<<"Close dialog";
  loop_ = false;
}
