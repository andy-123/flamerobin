/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


  $Id$

*/

#ifndef FR_METADATAITEMPROPERTIESFRAME_H
#define FR_METADATAITEMPROPERTIESFRAME_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/wxhtml.h>
#include <wx/aui/aui.h>

// wx 2.6 doesn't support wxHtmlCellEvent
#if wxCHECK_VERSION(2, 8, 0)
#include <wx/html/htmlwin.h>
#endif

#include "framemanager.h"
#include "core/Observer.h"
#include "gui/BaseFrame.h"
#include "gui/controls/PrintableHtmlWindow.h"
#include "metadata/metadataitem.h"
class MetadataItemPropertiesFrame;
//-----------------------------------------------------------------------------
class MetadataItemPropertiesPanel: public wxPanel, public Observer
{
private:
    enum { ptSummary, ptConstraints, ptDependencies, ptTriggers,
        ptTableIndices, ptDDL, ptPrivileges } pageTypeM;

    enum { HtmlWindowID = 42 };

    MetadataItem *objectM;
    bool htmlReloadRequestedM;
    PrintableHtmlWindow* html_window;

    // load page in idle handler, only request a reload in update()
    void requestLoadPage(bool showLoadingPage);
    void loadPage();
    void processCommand(wxString cmd, MetadataItem* object,
        wxString& htmlpage);
    void processHtmlCode(wxString& htmlpage, wxString htmlsource,
        MetadataItem* object = 0);

protected:
    virtual void removeSubject(Subject* subject);
    virtual void update();
public:
    MetadataItemPropertiesPanel(MetadataItemPropertiesFrame* parent,
        MetadataItem *object);
    virtual ~MetadataItemPropertiesPanel();

    MetadataItem* getObservedObject() const;
    void processHtmlFile(wxString fileName);
    void setPage(const wxString& type);
    void showIt();
    MetadataItemPropertiesFrame *getParentFrame();
private:
    // event handling
    void OnIdle(wxIdleEvent& event);

#if wxCHECK_VERSION(2, 8, 0)
    void OnHtmlCellHover(wxHtmlCellEvent &event);
    DECLARE_EVENT_TABLE()
#endif
};
//-----------------------------------------------------------------------------
class MetadataItemPropertiesFrame: public BaseFrame
{
private:
    wxString databaseNameM;

    // used to remember the value among calls to getStorageName(),
    // needed because it's not possible to access objectM
    // (see getStorageName()) after detaching from it.
    mutable wxString storageNameM;
    void setStorageName(MetadataItem *object);
protected:
    virtual const wxString getName() const;
    virtual const wxString getStorageName() const;
    virtual const wxRect getDefaultRect() const;
public:
    MetadataItemPropertiesFrame(wxWindow* parent, MetadataItem *object);
    virtual ~MetadataItemPropertiesFrame()
    {
        auiManagerM.UnInit();
    }

    enum { ID_notebook = 43 };

    void showPanel(wxWindow *panel, const wxString& title);
    void removePanel(wxWindow *panel);
    void setTabTitle(wxWindow *panel, const wxString& title);

    //MetadataItemPropertiesPanel *getItemPanel(MetadataItem *item);

    void OnClose(wxCloseEvent& event);
    void OnNotebookPageClose(wxAuiNotebookEvent& event);
    void OnNotebookPageChanged(wxAuiNotebookEvent& event);
private:
    wxAuiManager auiManagerM;
    wxAuiNotebook* notebookM;

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // FR_METADATAITEMPROPERTIESFRAME_H
