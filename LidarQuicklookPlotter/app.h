#ifndef APEX_APP_H
#define APEX_APP_H

#include<wx/wx.h>

class MyApp : public wxApp
{
public:
	virtual bool OnInit();
    virtual bool OnExceptionInMainLoop() override
    {
        try { throw; }
        catch (std::exception& e)
        {
            MessageBoxA(NULL, e.what(), "C++ Exception Caught", MB_OK);
        }
        return true;   // continue on. Return false to abort program
    }
};

#endif // APEX_APP_H
