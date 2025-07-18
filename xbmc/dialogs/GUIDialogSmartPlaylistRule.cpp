/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSmartPlaylistRule.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogSelect.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/LabelFormatter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"

#include <utility>

using enum CDatabaseQueryRule::FieldType;
using namespace KODI;

#define CONTROL_FIELD           15
#define CONTROL_OPERATOR        16
#define CONTROL_VALUE           17
#define CONTROL_OK              18
#define CONTROL_CANCEL          19
#define CONTROL_BROWSE          20

CGUIDialogSmartPlaylistRule::CGUIDialogSmartPlaylistRule(void)
    : CGUIDialog(WINDOW_DIALOG_SMART_PLAYLIST_RULE, "SmartPlaylistRule.xml")
{
  m_cancelled = false;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSmartPlaylistRule::~CGUIDialogSmartPlaylistRule() = default;

bool CGUIDialogSmartPlaylistRule::OnBack(int actionID)
{
  m_cancelled = true;
  return CGUIDialog::OnBack(actionID);
}

bool CGUIDialogSmartPlaylistRule::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_OK)
        OnOK();
      else if (iControl == CONTROL_CANCEL)
        OnCancel();
      else if (iControl == CONTROL_VALUE)
      {
        std::string parameter;
        OnEditChanged(iControl, parameter);
        m_rule.SetParameter(parameter);
      }
      else if (iControl == CONTROL_OPERATOR)
        OnOperator();
      else if (iControl == CONTROL_FIELD)
        OnField();
      else if (iControl == CONTROL_BROWSE)
        OnBrowse();
      return true;
    }
    break;

  case GUI_MSG_VALIDITY_CHANGED:
    CONTROL_ENABLE_ON_CONDITION(CONTROL_OK, message.GetParam1());
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSmartPlaylistRule::OnOK()
{
  m_cancelled = false;
  Close();
}

void CGUIDialogSmartPlaylistRule::OnBrowse()
{
  CFileItemList items;
  CMusicDatabase database;
  database.Open();
  CVideoDatabase videodatabase;
  videodatabase.Open();

  std::string basePath;
  if (PLAYLIST::CSmartPlaylist::IsMusicType(m_type))
    basePath = "musicdb://";
  else
    basePath = "videodb://";

  VideoDbContentType type = VideoDbContentType::MOVIES;
  if (m_type == "movies")
    basePath += "movies/";
  else if (m_type == "tvshows")
  {
    type = VideoDbContentType::TVSHOWS;
    basePath += "tvshows/";
  }
  else if (m_type == "musicvideos")
  {
    type = VideoDbContentType::MUSICVIDEOS;
    basePath += "musicvideos/";
  }
  else if (m_type == "episodes")
  {
    if (m_rule.m_field == FieldGenre || m_rule.m_field == FieldYear ||
        m_rule.m_field == FieldStudio)
      type = VideoDbContentType::TVSHOWS;
    else
      type = VideoDbContentType::EPISODES;
    basePath += "tvshows/";
  }

  int iLabel = 0;
  if (m_rule.m_field == FieldGenre)
  {
    if (m_type == "tvshows" ||
        m_type == "episodes" ||
        m_type == "movies")
      videodatabase.GetGenresNav(basePath + "genres/", items, type);
    else if (m_type == "songs" ||
             m_type == "albums" ||
             m_type == "artists" ||
             m_type == "mixed")
      database.GetGenresNav("musicdb://genres/",items);
    if (m_type == "musicvideos" ||
        m_type == "mixed")
    {
      CFileItemList items2;
      videodatabase.GetGenresNav("videodb://musicvideos/genres/", items2,
                                 VideoDbContentType::MUSICVIDEOS);
      items.Append(items2);
    }
    iLabel = 515;
  }
  else if (m_rule.m_field == FieldSource)
  {
    if (m_type == "songs" ||
      m_type == "albums" ||
      m_type == "artists" ||
      m_type == "mixed")
    {
      database.GetSourcesNav("musicdb://sources/", items);
      iLabel = 39030;
    }
  }
  else if (m_rule.m_field == FieldRole)
  {
    if (m_type == "artists" || m_type == "mixed")
    {
      database.GetRolesNav("musicdb://songs/", items);
      iLabel = 38033;
    }
  }
  else if (m_rule.m_field == FieldCountry)
  {
    videodatabase.GetCountriesNav(basePath, items, type);
    iLabel = 574;
  }
  else if (m_rule.m_field == FieldArtist || m_rule.m_field == FieldAlbumArtist)
  {
    if (PLAYLIST::CSmartPlaylist::IsMusicType(m_type))
      database.GetArtistsNav("musicdb://artists/", items, m_rule.m_field == FieldAlbumArtist, -1);
    if (m_type == "musicvideos" ||
        m_type == "mixed")
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoArtistsByName("", items2);
      items.Append(items2);
    }
    iLabel = 557;
  }
  else if (m_rule.m_field == FieldAlbum)
  {
    if (PLAYLIST::CSmartPlaylist::IsMusicType(m_type))
      database.GetAlbumsNav("musicdb://albums/", items);
    if (m_type == "musicvideos" ||
        m_type == "mixed")
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoAlbumsByName("", items2);
      items.Append(items2);
    }
    iLabel = 558;
  }
  else if (m_rule.m_field == FieldActor)
  {
    videodatabase.GetActorsNav(basePath + "actors/",items,type);
    iLabel = 20337;
  }
  else if (m_rule.m_field == FieldYear)
  {
    if (PLAYLIST::CSmartPlaylist::IsMusicType(m_type))
      database.GetYearsNav("musicdb://years/", items);
    if (PLAYLIST::CSmartPlaylist::IsVideoType(m_type))
    {
      CFileItemList items2;
      videodatabase.GetYearsNav(basePath + "years/", items2, type);
      items.Append(items2);
    }
    iLabel = 562;
  }
  else if (m_rule.m_field == FieldOrigYear)
  {
    database.GetYearsNav("musicdb://originalyears/", items);
    iLabel = 38078;
  }
  else if (m_rule.m_field == FieldDirector)
  {
    videodatabase.GetDirectorsNav(basePath + "directors/", items, type);
    iLabel = 20339;
  }
  else if (m_rule.m_field == FieldStudio)
  {
    videodatabase.GetStudiosNav(basePath + "studios/", items, type);
    iLabel = 572;
  }
  else if (m_rule.m_field == FieldWriter)
  {
    videodatabase.GetWritersNav(basePath, items, type);
    iLabel = 20417;
  }
  else if (m_rule.m_field == FieldTvShowTitle ||
          (m_type == "tvshows" && m_rule.m_field == FieldTitle))
  {
    videodatabase.GetTvShowsNav(basePath + "titles/", items);
    iLabel = 20343;
  }
  else if (m_rule.m_field == FieldTitle)
  {
    if (m_type == "songs" || m_type == "mixed")
    {
      database.GetSongsNav("musicdb://songs/", items, -1, -1, -1);
      iLabel = 134;
    }
    if (m_type == "movies")
    {
      videodatabase.GetMoviesNav(basePath + "titles/", items);
      iLabel = 20342;
    }
    if (m_type == "episodes")
    {
      videodatabase.GetEpisodesNav(basePath + "titles/-1/-1/", items);
      // we need to replace the db label (<season>x<episode> <title>) with the title only
      CLabelFormatter format("%T", "");
      for (int i = 0; i < items.Size(); i++)
        format.FormatLabel(items[i].get());
      iLabel = 20360;
    }
    if (m_type == "musicvideos" || m_type == "mixed")
    {
      videodatabase.GetMusicVideosNav(basePath + "titles/", items);
      iLabel = 20389;
    }
  }
  else if (m_rule.m_field == FieldPlaylist || m_rule.m_field == FieldVirtualFolder)
  {
    // use filebrowser to grab another smart playlist

    // Note: This can cause infinite loops (playlist that refers to the same playlist) but I don't
    //       think there's any decent way to deal with this, as the infinite loop may be an arbitrary
    //       number of playlists deep, eg playlist1 -> playlist2 -> playlist3 ... -> playlistn -> playlist1
    if (PLAYLIST::CSmartPlaylist::IsVideoType(m_type))
      XFILE::CDirectory::GetDirectory("special://videoplaylists/", items, ".xsp", XFILE::DIR_FLAG_NO_FILE_DIRS);
    if (PLAYLIST::CSmartPlaylist::IsMusicType(m_type))
    {
      CFileItemList items2;
      XFILE::CDirectory::GetDirectory("special://musicplaylists/", items2, ".xsp", XFILE::DIR_FLAG_NO_FILE_DIRS);
      items.Append(items2);
    }

    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      PLAYLIST::CSmartPlaylist playlist;
      // don't list unloadable smartplaylists or any referenceable smartplaylists
      // which do not match the type of the current smartplaylist
      if (!playlist.Load(item->GetPath()) ||
          (m_rule.m_field == FieldPlaylist &&
           (!PLAYLIST::CSmartPlaylist::CheckTypeCompatibility(m_type, playlist.GetType()) ||
            (!playlist.GetGroup().empty() || playlist.IsGroupMixed()))))
      {
        items.Remove(i);
        i -= 1;
        continue;
      }

      if (!playlist.GetName().empty())
        item->SetLabel(playlist.GetName());
    }
    iLabel = 559;
  }
  else if (m_rule.m_field == FieldPath)
  {
    std::vector<CMediaSource> sources;
    if (m_type == "songs" || m_type == "mixed")
      sources = *CMediaSourceSettings::GetInstance().GetSources("music");
    if (PLAYLIST::CSmartPlaylist::IsVideoType(m_type))
    {
      std::vector<CMediaSource> sources2 = *CMediaSourceSettings::GetInstance().GetSources("video");
      sources.insert(sources.end(),sources2.begin(),sources2.end());
    }
    CServiceBroker::GetMediaManager().GetLocalDrives(sources);

    std::string path = m_rule.GetParameter();
    CGUIDialogFileBrowser::ShowAndGetDirectory(sources, g_localizeStrings.Get(657), path, false);
    if (!m_rule.m_parameter.empty())
      m_rule.m_parameter.clear();
    if (!path.empty())
      m_rule.m_parameter.emplace_back(std::move(path));

    UpdateButtons();
    return;
  }
  else if (m_rule.m_field == FieldSet)
  {
    videodatabase.GetSetsNav("videodb://movies/sets/", items, VideoDbContentType::MOVIES);
    iLabel = 20434;
  }
  else if (m_rule.m_field == FieldTag)
  {
    VideoDbContentType type = VideoDbContentType::MOVIES;
    if (m_type == "tvshows" ||
        m_type == "episodes")
      type = VideoDbContentType::TVSHOWS;
    else if (m_type == "musicvideos")
      type = VideoDbContentType::MUSICVIDEOS;
    else if (m_type != "movies")
      return;

    videodatabase.GetTagsNav(basePath + "tags/", items, type);
    iLabel = 20459;
  }
  else
  { //! @todo Add browseability in here.
    assert(false);
  }

  // sort the items
  items.Sort(SortByLabel, SortOrderAscending, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);

  CGUIDialogSelect* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetItems(items);
  std::string strHeading =
      StringUtils::Format(g_localizeStrings.Get(13401), g_localizeStrings.Get(iLabel));
  pDialog->SetHeading(CVariant{std::move(strHeading)});
  pDialog->SetMultiSelection(m_rule.m_field != FieldPlaylist && m_rule.m_field != FieldVirtualFolder);

  if (!m_rule.m_parameter.empty())
    pDialog->SetSelected(m_rule.m_parameter);

  pDialog->Open();
  if (pDialog->IsConfirmed())
  {
    m_rule.m_parameter.clear();
    for (int i : pDialog->GetSelectedItems())
      m_rule.m_parameter.push_back(items.Get(i)->GetLabel());

    UpdateButtons();
  }
  pDialog->Reset();
}

namespace
{
std::pair<std::string, CDatabaseQueryRule::SearchOperator> OperatorLabel(
    CDatabaseQueryRule::SearchOperator op)
{
  return std::make_pair(PLAYLIST::CSmartPlaylistRule::GetLocalizedOperator(op), op);
}
} // unnamed namespace

std::vector<std::pair<std::string, CDatabaseQueryRule::SearchOperator>>
CGUIDialogSmartPlaylistRule::GetValidOperators(const PLAYLIST::CSmartPlaylistRule& rule) const
{
  std::vector<std::pair<std::string, CDatabaseQueryRule::SearchOperator>> labels;
  switch (rule.GetFieldType(rule.m_field))
  {
    using enum CDatabaseQueryRule::SearchOperator;
    case TEXT_FIELD:
      // text fields - add the usual comparisons
      labels.push_back(OperatorLabel(OPERATOR_EQUALS));
      labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_EQUAL));
      labels.push_back(OperatorLabel(OPERATOR_CONTAINS));
      labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_CONTAIN));
      labels.push_back(OperatorLabel(OPERATOR_STARTS_WITH));
      labels.push_back(OperatorLabel(OPERATOR_ENDS_WITH));
      break;

    case REAL_FIELD:
    case NUMERIC_FIELD:
    case SECONDS_FIELD:
      // numerical fields - less than greater than
      labels.push_back(OperatorLabel(OPERATOR_EQUALS));
      labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_EQUAL));
      labels.push_back(OperatorLabel(OPERATOR_GREATER_THAN));
      labels.push_back(OperatorLabel(OPERATOR_LESS_THAN));
      break;

    case DATE_FIELD:
      // date field
      labels.push_back(OperatorLabel(OPERATOR_AFTER));
      labels.push_back(OperatorLabel(OPERATOR_BEFORE));
      labels.push_back(OperatorLabel(OPERATOR_IN_THE_LAST));
      labels.push_back(OperatorLabel(OPERATOR_NOT_IN_THE_LAST));
      break;

    case PLAYLIST_FIELD:
    case TEXTIN_FIELD:
      labels.push_back(OperatorLabel(OPERATOR_EQUALS));
      labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_EQUAL));
      break;

    case BOOLEAN_FIELD:
      labels.push_back(OperatorLabel(OPERATOR_TRUE));
      labels.push_back(OperatorLabel(OPERATOR_FALSE));
      break;
  }
  return labels;
}

void CGUIDialogSmartPlaylistRule::OnCancel()
{
  m_cancelled = true;
  Close();
}

void CGUIDialogSmartPlaylistRule::OnField()
{
  const auto fields = PLAYLIST::CSmartPlaylistRule::GetFields(m_type);
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  dialog->SetHeading(CVariant{20427});
  int selected = -1;
  for (auto field = fields.begin(); field != fields.end(); field++)
  {
    dialog->Add(PLAYLIST::CSmartPlaylistRule::GetLocalizedField(*field));
    if (*field == m_rule.m_field)
      selected = std::distance(fields.begin(), field);
  }
  if (selected > -1)
    dialog->SetSelected(selected);
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  // check if selection has changed
  if (!dialog->IsConfirmed() || newSelected < 0 || newSelected == selected)
    return;

  m_rule.m_field = fields[newSelected];
  // check if operator is still valid. if not, reset to first valid one
  const std::vector<std::pair<std::string, CDatabaseQueryRule::SearchOperator>> validOperators =
      GetValidOperators(m_rule);
  bool isValid = false;
  for (auto op : validOperators)
    if (std::get<0>(op) == std::get<0>(OperatorLabel(m_rule.m_operator)))
      isValid = true;
  if (!isValid)
    m_rule.m_operator = std::get<1>(validOperators[0]);

  m_rule.SetParameter("");
  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::OnOperator()
{
  const auto labels = GetValidOperators(m_rule);
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  dialog->SetHeading(CVariant{ 16023 });
  for (auto label : labels)
    dialog->Add(std::get<0>(label));
  dialog->SetSelected(PLAYLIST::CSmartPlaylistRule::GetLocalizedOperator(m_rule.m_operator));
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  // check if selection has changed
  if (!dialog->IsConfirmed() || newSelected < 0)
    return;

  m_rule.m_operator = std::get<1>(labels[newSelected]);
  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::UpdateButtons()
{
  if (m_rule.m_field == 0)
    m_rule.m_field = PLAYLIST::CSmartPlaylistRule::GetFields(m_type)[0];
  SET_CONTROL_LABEL(CONTROL_FIELD, PLAYLIST::CSmartPlaylistRule::GetLocalizedField(m_rule.m_field));

  const CDatabaseQueryRule::FieldType fieldType = m_rule.GetFieldType(m_rule.m_field);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_VALUE, fieldType != BOOLEAN_FIELD);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BROWSE,
                              PLAYLIST::CSmartPlaylistRule::IsFieldBrowseable(m_rule.m_field));

  SET_CONTROL_LABEL(CONTROL_OPERATOR, std::get<0>(OperatorLabel(m_rule.m_operator)));

  // update label2 appropriately
  SET_CONTROL_LABEL2(CONTROL_VALUE, m_rule.GetParameter());
  CGUIEditControl::INPUT_TYPE type = CGUIEditControl::INPUT_TYPE_TEXT;

  switch (fieldType)
  {
    case TEXT_FIELD:
    case PLAYLIST_FIELD:
    case TEXTIN_FIELD:
    case REAL_FIELD:
    case NUMERIC_FIELD:
      type = CGUIEditControl::INPUT_TYPE_TEXT;
      break;
    case DATE_FIELD:
      if (m_rule.m_operator == CDatabaseQueryRule::SearchOperator::OPERATOR_IN_THE_LAST ||
          m_rule.m_operator == CDatabaseQueryRule::SearchOperator::OPERATOR_NOT_IN_THE_LAST)
        type = CGUIEditControl::INPUT_TYPE_TEXT;
      else
        type = CGUIEditControl::INPUT_TYPE_DATE;
      break;
    case SECONDS_FIELD:
      type = CGUIEditControl::INPUT_TYPE_SECONDS;
      break;
    case BOOLEAN_FIELD:
      type = CGUIEditControl::INPUT_TYPE_NUMBER;
      break;
  }
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_VALUE, type, 21420);
}

void CGUIDialogSmartPlaylistRule::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  UpdateButtons();

  CGUIEditControl *editControl = dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_VALUE));
  if (editControl != NULL)
    editControl->SetInputValidation(PLAYLIST::CSmartPlaylistRule::Validate, &m_rule);
}

void CGUIDialogSmartPlaylistRule::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // reset field spincontrolex
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_FIELD);
  // reset operator spincontrolex
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_OPERATOR);
}

bool CGUIDialogSmartPlaylistRule::EditRule(PLAYLIST::CSmartPlaylistRule& rule,
                                           const std::string& type)
{
  CGUIDialogSmartPlaylistRule *editor = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSmartPlaylistRule>(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
  if (!editor) return false;

  editor->m_rule = rule;
  editor->m_type = type;
  editor->Open();
  rule = editor->m_rule;
  return !editor->m_cancelled;
}

