/*
 * ct_app.cc
 *
 * Copyright 2017-2020 Giuseppe Penone <giuspen@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <glib/gstdio.h>
#include "ct_app.h"
#include "ct_pref_dlg.h"
#include "config.h"

CtConfig* CtApp::P_ctCfg{nullptr};
CtActions* CtApp::P_ctActions{nullptr};
Glib::RefPtr<Gtk::IconTheme> CtApp::R_icontheme;
CtTmp* CtApp::P_ctTmp{nullptr};
Glib::RefPtr<Gtk::TextTagTable> CtApp::R_textTagTable;
Glib::RefPtr<Gsv::LanguageManager> CtApp::R_languageManager;
Glib::RefPtr<Gsv::StyleSchemeManager> CtApp::R_styleSchemeManager;
Glib::RefPtr<Gtk::CssProvider> CtApp::R_cssProvider;

CtApp::CtApp() : Gtk::Application("com.giuspen.cherrytree", Gio::APPLICATION_HANDLES_OPEN)
{
    Gsv::init();

    if (nullptr == P_ctCfg)
    {
        P_ctCfg = new CtConfig();
        //std::cout << P_ctCfg->specialChars.size() << "\t" << P_ctCfg->specialChars << std::endl;
    }
    if (nullptr == P_ctActions)
    {
        P_ctActions = new CtActions();
    }
    if (not R_icontheme)
    {
        _iconthemeInit();
    }
    if (nullptr == P_ctTmp)
    {
        P_ctTmp = new CtTmp();
        //std::cout << P_ctTmp->get_root_dirpath() << std::endl;
    }
    if (not R_textTagTable)
    {
        R_textTagTable = Gtk::TextTagTable::create();
    }
    if (not R_languageManager)
    {
        R_languageManager = Gsv::LanguageManager::create();
    }
    if (not R_styleSchemeManager)
    {
        R_styleSchemeManager = Gsv::StyleSchemeManager::create();
    }
    if (not R_cssProvider)
    {
        R_cssProvider = Gtk::CssProvider::create();
    }

    _pCtMenu = new CtMenu();
    _pCtMenu->init_actions(this, P_ctActions);
}

CtApp::~CtApp()
{
    //std::cout << "~CtApp()" << std::endl;
    delete P_ctCfg;
    P_ctCfg = nullptr;

    delete P_ctActions;
    P_ctActions = nullptr;

    delete P_ctTmp;
    P_ctTmp = nullptr;

    delete _pCtMenu;
}

Glib::RefPtr<CtApp> CtApp::create()
{
    return Glib::RefPtr<CtApp>(new CtApp());
}

void CtApp::_printHelpMessage()
{
    std::cout << "Usage: " << GETTEXT_PACKAGE << " [filepath.ctd|.ctb|.ctz|.ctx]" << std::endl;
}

void CtApp::_printGresourceIcons()
{
    for (std::string &str_icon : Gio::Resource::enumerate_children_global("/icons/", Gio::ResourceLookupFlags::RESOURCE_LOOKUP_FLAGS_NONE))
    {
        std::cout << str_icon << std::endl;
    }
}

void CtApp::_iconthemeInit()
{
    R_icontheme = Gtk::IconTheme::get_default();
    R_icontheme->add_resource_path("/icons/");
    //_printGresourceIcons();
}

CtMainWin* CtApp::create_appwindow()
{
    CtMainWin* pCtMainWin = new CtMainWin(_pCtMenu);
    CtApp::P_ctActions->init(pCtMainWin);

    add_window(*pCtMainWin);

    pCtMainWin->signal_hide().connect(sigc::bind<CtMainWin*>(sigc::mem_fun(*this, &CtApp::on_hide_window), pCtMainWin));
    return pCtMainWin;
}

CtMainWin* CtApp::get_main_win()
{
    auto windows_list = get_windows();
    if (windows_list.size() > 0)
        return dynamic_cast<CtMainWin*>(windows_list[0]);
    return create_appwindow();
}

void CtApp::on_activate()
{
    // app run without arguments
    auto pAppWindow = create_appwindow();
    pAppWindow->present();

    if (not CtApp::P_ctCfg->recentDocsFilepaths.empty())
    {
        Glib::RefPtr<Gio::File> r_file = Gio::File::create_for_path(CtApp::P_ctCfg->recentDocsFilepaths.front());
        if (r_file->query_exists())
        {
            if (not pAppWindow->read_nodes_from_gio_file(r_file, false/*isImport*/))
            {
                _printHelpMessage();
            }
        }
        else
        {
            std::cout << "? not found " << CtApp::P_ctCfg->recentDocsFilepaths.front() << std::endl;
            CtApp::P_ctCfg->recentDocsFilepaths.move_or_push_back(CtApp::P_ctCfg->recentDocsFilepaths.front());
            pAppWindow->set_menu_items_recent_documents();
        }
    }
}

void CtApp::on_hide_window(CtMainWin* pCtMainWin)
{
    pCtMainWin->config_update_data_from_curr_status();
    P_ctCfg->write_to_file();
    delete pCtMainWin;
}

void CtApp::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& /*hint*/)
{
    // app run with arguments
    CtMainWin* pAppWindow = get_main_win();

    for (const Glib::RefPtr<Gio::File>& r_file : files)
    {
        if (r_file->query_exists())
        {
            if (not pAppWindow->read_nodes_from_gio_file(r_file, false/*isImport*/))
            {
                _printHelpMessage();
            }
        }
        else
        {
            std::cout << "!! Missing file " << r_file->get_path() << std::endl;
        }
    }

    pAppWindow->present();
}

void CtApp::quit_application()
{
    quit();
}

void CtApp::dialog_preferences()
{
    CtPrefDlg prefDlg(get_main_win(), _pCtMenu);
    prefDlg.show();
    prefDlg.run();
    prefDlg.hide();
}



CtTmp::CtTmp()
{
}

CtTmp::~CtTmp()
{
    //std::cout << "~CtTmp()" << std::endl;
    for (const auto& currPair : _mapHiddenFiles)
    {
        if (g_file_test(currPair.second, G_FILE_TEST_IS_REGULAR) and (0 != g_remove(currPair.second)))
        {
            std::cerr << "!! g_remove" << std::endl;
        }
        g_free(currPair.second);
    }
    for (const auto& currPair : _mapHiddenDirs)
    {
        if (g_file_test(currPair.second, G_FILE_TEST_IS_DIR) and (0 != g_rmdir(currPair.second)))
        {
            std::cerr << "!! g_rmdir" << std::endl;
        }
        g_free(currPair.second);
    }
}

const gchar* CtTmp::getHiddenDirPath(const std::string& visiblePath)
{
    if (not _mapHiddenDirs.count(visiblePath))
    {
        _mapHiddenDirs[visiblePath] = g_dir_make_tmp(nullptr, nullptr);
    }
    return _mapHiddenDirs.at(visiblePath);
}

const gchar* CtTmp::getHiddenFilePath(const std::string& visiblePath)
{
    if (not _mapHiddenFiles.count(visiblePath))
    {
        const gchar* tempDir{getHiddenDirPath(visiblePath)};
        std::string basename{Glib::path_get_basename(visiblePath)};
        if (Glib::str_has_suffix(basename, ".ctx"))
        {
            basename.replace(basename.end()-1, basename.end(), "b");
        }
        else if (Glib::str_has_suffix(basename, ".ctz"))
        {
            basename.replace(basename.end()-1, basename.end(), "d");
        }
        _mapHiddenFiles[visiblePath] = g_build_filename(tempDir, basename.c_str(), NULL);
    }
    return _mapHiddenFiles.at(visiblePath);
}
