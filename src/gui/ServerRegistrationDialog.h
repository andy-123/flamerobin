/*
The contents of this file are subject to the Initial Developer's Public
License Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License here:
http://www.flamerobin.org/license.html.

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations under
the License.

The Original Code is FlameRobin (TM).

The Initial Developer of the Original Code is Milan Babuskov.

Portions created by the original developer
are Copyright (C) 2004 Milan Babuskov.

All Rights Reserved.

$Id$

Contributor(s): Michael Hieke, Nando Dessena
*/

#ifndef SERVERREGISTRATIONDIALOG_H
#define SERVERREGISTRATIONDIALOG_H

#include <wx/wx.h>
#include "metadata/server.h"
#include "BaseDialog.h"


class ServerRegistrationDialog: public BaseDialog {
public:
    enum {
        ID_textctrl_hostname = 100,
        ID_button_ok = wxID_OK ,
        ID_button_cancel = wxID_CANCEL
    };

    void setServer(YServer *s);

    // events
    void OnSettingsChange(wxCommandEvent& event);
    void OnOkButtonClick(wxCommandEvent& event);
    void OnCancelButtonClick(wxCommandEvent& event);

    ServerRegistrationDialog(wxWindow* parent, int id, const wxString& title, 
        const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, 
        long style=wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);

private:
    YServer* serverM;

    void do_layout();
    void set_properties();
    void updateButtons();

protected:
    wxStaticText* label_hostname;
    wxTextCtrl* text_ctrl_hostname;
    wxStaticText* label_portnumber;
    wxTextCtrl* text_ctrl_portnumber;
    wxButton* button_ok;
    wxButton* button_cancel;

    DECLARE_EVENT_TABLE()
    virtual const std::string getName() const;
};

#endif // SERVERREGISTRATIONDIALOG_H
