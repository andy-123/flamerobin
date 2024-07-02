//	internal utilities

/*	(C) Copyright 2000-2006 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)

    The contents of this file are subject to the IBPP License (the "License");
    you may not use this file except in compliance with the License.  You may
    obtain a copy of the License at http://www.ibpp.org or in the 'license.txt'
    file which must have been distributed along with this file.

    This software, distributed under the License, is distributed on an "AS IS"
    basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
    License for the specific language governing rights and limitations
    under the License.
*/

#include "_ibpp.h"

#include <algorithm>

using namespace std;

namespace IBPP
{

string StrToUpper(const string &s)
{
    string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);

    return result;
}

}