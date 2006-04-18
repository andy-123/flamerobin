/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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
//-----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/ProgressDialog.h"
#include "styleguide.h"
//-----------------------------------------------------------------------------
// ProgressDialog
ProgressDialog::ProgressDialog(wxWindow* parent, const wxString& title,
        unsigned int levelCount, const wxPoint& pos, const wxSize& size)
    : BaseDialog(parent, wxID_ANY, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
    canceledM = false;
    levelCountM = levelCount;
    SetExtraStyle(GetExtraStyle() | wxWS_EX_TRANSIENT);

    createControls();
    layoutControls();

    winDisablerM = 0;
    enableOtherWindows(false);
    Show();
    Enable();
    doUpdate();
}
//-----------------------------------------------------------------------------
ProgressDialog::~ProgressDialog()
{
    enableOtherWindows(true);
}
//-----------------------------------------------------------------------------
void ProgressDialog::createControls()
{
    int gaugeHeight = wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y);
    for (unsigned int i = 1; i <= levelCountM; i++)
    {
        wxStaticText* label = new wxStaticText(getControlsPanel(), wxID_ANY,
            wxEmptyString, wxDefaultPosition, wxDefaultSize,
            // don't resize the label, keep the same width as the gauge
            wxST_NO_AUTORESIZE);
        labelsM.push_back(label);
        wxGauge* gauge = new wxGauge(getControlsPanel(), wxID_ANY, 100,
            wxDefaultPosition, wxSize(300, gaugeHeight),
            wxGA_HORIZONTAL | wxGA_SMOOTH);
        gaugesM.push_back(gauge);
    }
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}
//-----------------------------------------------------------------------------
void ProgressDialog::doUpdate()
{
    Update();
#ifdef __WXGTK__
    // TODO: find the right incantation to update and show controls
#endif
}
//-----------------------------------------------------------------------------
void ProgressDialog::enableOtherWindows(bool enable)
{
     if (!enable && (0 == winDisablerM))
        winDisablerM = new wxWindowDisabler(this);
    else if (enable)
    {
        delete winDisablerM;
        winDisablerM = 0;
    }
}
//-----------------------------------------------------------------------------
void ProgressDialog::layoutControls()
{
    wxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    int dyLarge = styleguide().getUnrelatedControlMargin(wxVERTICAL);
    int dySmall = styleguide().getRelatedControlMargin(wxVERTICAL);
    for (unsigned int i = 0; i < levelCountM; i++)
    {
        if (i > 0)
            sizerControls->AddSpacer(dyLarge);
        sizerControls->Add(labelsM[i], 0, wxEXPAND);
        sizerControls->AddSpacer(dySmall);
        sizerControls->Add(gaugesM[i], 0, wxEXPAND);
    }

    wxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_cancel);
    sizerButtons->Add(0, 0, 1, wxEXPAND);

    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
wxGauge* ProgressDialog::getGaugeForLevel(unsigned int progressLevel)
{
    if (progressLevel > 0 && progressLevel <= gaugesM.size())
        return gaugesM[progressLevel - 1];
    return 0;
}
//-----------------------------------------------------------------------------
wxStaticText* ProgressDialog::getLabelForLevel(unsigned int progressLevel)
{
    if (progressLevel > 0 && progressLevel <= labelsM.size())
        return labelsM[progressLevel - 1];
    return 0;
}
//-----------------------------------------------------------------------------
inline bool ProgressDialog::isValidProgressLevel(unsigned int progressLevel)
{
    return progressLevel > 0 && progressLevel <= levelCountM;
}
//-----------------------------------------------------------------------------
void ProgressDialog::setCanceled()
{
    if (!canceledM)
    {
        canceledM = true;
        button_cancel->Enable(false);
        setProgressMessage(_("Cancelling..."));
        doUpdate();
    }
}
//-----------------------------------------------------------------------------
void ProgressDialog::setGaugeIndeterminate(wxGauge* gauge,
    bool /* indeterminate */ )
{
    if (gauge)
    {
    // TODO
    }
}
//-----------------------------------------------------------------------------
bool ProgressDialog::Show(bool show)
{
    if (!show)
        enableOtherWindows(true);
    return BaseDialog::Show(show);
}
//-----------------------------------------------------------------------------
// ProgressIndicator methods
bool ProgressDialog::isCanceled()
{
    wxYieldIfNeeded();
    return canceledM;
}
//-----------------------------------------------------------------------------
void ProgressDialog::initProgress(wxString progressMsg,
    unsigned int maxPosition, unsigned int startingPosition,
    unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (label && gauge)
    {
        label->SetLabel(progressMsg);
        setGaugeIndeterminate(gauge, false);
        gauge->SetRange(maxPosition);
        gauge->SetValue(startingPosition);
        doUpdate();
    }
}
//-----------------------------------------------------------------------------
void ProgressDialog::initProgressIndeterminate(wxString progressMsg,
    unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (label && gauge)
    {
        label->SetLabel(progressMsg);
        setGaugeIndeterminate(gauge, true);
        doUpdate();
    }
}
//-----------------------------------------------------------------------------
void ProgressDialog::setProgressMessage(wxString progressMsg,
    unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    if (label)
        label->SetLabel(progressMsg);
    doUpdate();
}
//-----------------------------------------------------------------------------
void ProgressDialog::setProgressPosition(unsigned int currentPosition,
    unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (gauge)
        gauge->SetValue(currentPosition);
    doUpdate();
}
//-----------------------------------------------------------------------------
void ProgressDialog::stepProgress(int stepAmount, unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (gauge)
    {
        int pos = gauge->GetValue() + stepAmount;
        int maxPos = gauge->GetRange();
        gauge->SetValue((pos < 0) ? 0 : (pos > maxPos ? maxPos : pos));
        doUpdate();
    }
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(ProgressDialog, BaseDialog)
    EVT_BUTTON(wxID_CANCEL, ProgressDialog::OnCancelButtonClick)
    EVT_CLOSE(ProgressDialog::OnClose)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void ProgressDialog::OnCancelButtonClick(wxCommandEvent& WXUNUSED(event))
{
    setCanceled();
}
//-----------------------------------------------------------------------------
void ProgressDialog::OnClose(wxCloseEvent& event)
{
    setCanceled();
    event.Veto();
}
//-----------------------------------------------------------------------------
