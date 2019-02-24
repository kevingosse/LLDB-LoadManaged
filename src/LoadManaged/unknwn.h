// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
//

//
// ===========================================================================
// File: unknwn.h
//
// ===========================================================================
// simplified unknwn.h for PAL

//#include "rpc.h"
//#include "rpcndr.h"
#include "mstypes.h"

#ifndef __IUnknown_INTERFACE_DEFINED__
#define __IUnknown_INTERFACE_DEFINED__

typedef GUID IID;
#define REFIID const IID &

typedef struct IUnknown IUnknown;

typedef /* [unique] */ IUnknown *LPUNKNOWN;

// 00000000-0000-0000-C000-000000000046
EXTERN_C const IID IID_IUnknown;

//MIDL_INTERFACE("00000000-0000-0000-C000-000000000046")
struct IUnknown
{
    virtual HRESULT QueryInterface(
        REFIID riid,
        void **ppvObject) = 0;

    virtual ULONG AddRef( void) = 0;

    virtual ULONG Release( void) = 0;

   /* template<class Q>
    HRESULT
    QueryInterface(Q** pp)
    {
        return QueryInterface(__uuidof(Q), (void **)pp);
    }*/
};

#endif // __IUnknown_INTERFACE_DEFINED__

#ifndef __IClassFactory_INTERFACE_DEFINED__
#define __IClassFactory_INTERFACE_DEFINED__

// 00000001-0000-0000-C000-000000000046
extern "C" const IID IID_IClassFactory;

struct IClassFactory : public IUnknown
{
    virtual HRESULT CreateInstance(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObject) = 0;

    virtual HRESULT LockServer(
        BOOL fLock) = 0;
};

#endif // __IClassFactory_INTERFACE_DEFINED__