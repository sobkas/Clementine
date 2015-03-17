/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>
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

#include "addpodcastbyurl.h"

#include <QClipboard>
#include <QNetworkReply>
#include <QMessageBox>

#include "podcastdiscoverymodel.h"
#include "podcasturlloader.h"
#include "ui_addpodcastbyurl.h"
#include "core/closure.h"
#include "core/logging.h"

AddPodcastByUrl::AddPodcastByUrl(Application* app, QWidget* parent)
    : AddPodcastPage(app, parent),
      ui_(new Ui_AddPodcastByUrl),
      loader_(new PodcastUrlLoader(this)) {
  ui_->setupUi(this);
  connect(ui_->go, SIGNAL(clicked()), SLOT(GoClicked()));
  connect(this, SIGNAL(close_dialog()), loader_, SLOT(close_dialog()));
}

AddPodcastByUrl::~AddPodcastByUrl() {
  qLog(Debug) << "~AddPodcastByUrl() was run";
  delete ui_;
}

void AddPodcastByUrl::SetUrlAndGo(const QUrl& url) {
  ui_->url->setText(url.toString());
  GoClicked();
}

void AddPodcastByUrl::GoClicked() {
  emit Busy(true);
  model()->clear();

  PodcastUrlLoaderReply* reply = loader_->Load(ui_->url->text());
  ui_->url->setText(reply->url().toString());

  NewClosure(reply, SIGNAL(Finished(bool)), this,
             SLOT(RequestFinished(PodcastUrlLoaderReply*)), reply);
}

void AddPodcastByUrl::RequestFinished(PodcastUrlLoaderReply* reply) {
  reply->deleteLater();

  emit Busy(false);

  if (!reply->is_success()) {
    QMessageBox::warning(this, tr("Failed to load podcast"),
                         reply->error_text(), QMessageBox::Close);
    return;
  }

  for (const Podcast& podcast : reply->podcast_results()) {
    model()->appendRow(model()->CreatePodcastItem(podcast));
  }
}

void AddPodcastByUrl::Show() {
  ui_->url->setFocus();

  const QClipboard* clipboard = QApplication::clipboard();
  QStringList contents;
  contents << clipboard->text(QClipboard::Selection)
           << clipboard->text(QClipboard::Clipboard);

  for (const QString& content : contents) {
    if (content.contains("://")) {
      ui_->url->setText(content);
      return;
    }
  }
}
