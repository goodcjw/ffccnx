/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jiwen Cai <jwcai@cs.ucla.edu>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCCNxError.h"
#include "nsCCNxTransport.h"

#include "nsNetSegmentUtils.h"
#include "nsStreamUtils.h"

#include "nsIPipe.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo* gCCNxLog;
#endif
#define LOG(args)         PR_LOG(gCCNxLog, PR_LOG_DEBUG, args)

NS_IMPL_THREADSAFE_ISUPPORTS1(nsCCNxTransport,
                              nsITransport)

nsCCNxTransport::nsCCNxTransport()
    : mLock("nsCCNxTransport.mLock"),
      mCCNx(nsnull),
      mCCNxfetch(nsnull),
      mCCNxname(nsnull),
      mCCNxtmpl(nsnull),
      mCCNxstream(nsnull),
      mCCNxref(0),
      mCCNxonline(false),
      mInputClosed(true),
      mInput(this) {

  LOG(("create nsCCNxTransport @%p", this));
}

nsCCNxTransport::~nsCCNxTransport() {
  mService->Shutdown();
  LOG(("destroy nsCCNxTransport @%p", this));
}

nsresult
nsCCNxTransport::Init(const char *ndnName) {
  // the current implementation only allows one ccn name
  int res;
  mService = new nsCCNxTransportService();
  mService->Init();

  // create ccn connection
  mCCNx = ccn_create();
  res = ccn_connect(mCCNx, NULL);
  if (res < 0) {
    ccn_destroy(&mCCNx);
    return NS_ERROR_CCND_UNAVAIL;
  }

  // create name buffer
  mCCNxname = ccn_charbuf_create();
  mCCNxname->length = 0;
  res = ccn_name_from_uri(mCCNxname, ndnName);
  if (res < 0) {
    ccn_destroy(&mCCNx);
    ccn_charbuf_destroy(&mCCNxname);
    return NS_ERROR_CCND_INVALID_NAME;
  }

  // initialize ccn fetch
  mCCNxfetch = ccn_fetch_new(mCCNx);

  // initialize interest template (mCCNxtmpl)
  CCNX_MakeTemplate(0);

  // initialize ccn stream
  // XXX size of buffer is hard coded here, which must be wrong
  // copied from ccnwget
  // maxBufs = 4
  // assumeFixed = 0
  mCCNxstream = ccn_fetch_open(mCCNxfetch, mCCNxname, ndnName, 
                               mCCNxtmpl, 4, CCN_V_HIGHEST, 0);

  mCCNxonline = true;
  return NS_OK;
}

NS_IMETHODIMP
nsCCNxTransport::OpenInputStream(PRUint32 flags,
                                 PRUint32 segsize,
                                 PRUint32 segcount,
                                 nsIInputStream **result) {

  LOG(("nsCCNxTransport::OpenInputStream [this=%x flags=%x]\n",
       this, flags));
  NS_ENSURE_TRUE(!mInput.IsReferenced(), NS_ERROR_UNEXPECTED);

  nsresult rv;
  nsCOMPtr<nsIAsyncInputStream> pipeIn;

  if (!(flags & OPEN_UNBUFFERED) || (flags & OPEN_BLOCKING)) {
    bool openBlocking = (flags & OPEN_BLOCKING);

    net_ResolveSegmentParams(segsize, segcount);
    nsIMemory *segalloc = net_GetSegmentAlloc(segsize);

    // create a pipe
    nsCOMPtr<nsIAsyncOutputStream> pipeOut;
    rv = NS_NewPipe2(getter_AddRefs(pipeIn), getter_AddRefs(pipeOut),
                     !openBlocking, true, segsize, segcount, segalloc);
    if (NS_FAILED(rv)) return rv;
    // no callback for NS_AsyncCopy, the output will be directly push into the 
    // pipe the thread at the other size of the pipe (pipeOut's OnInputStreamReady)
    // should deal with callback.
    rv = NS_AsyncCopy(&mInput, pipeOut, mService,
                      NS_ASYNCCOPY_VIA_WRITESEGMENTS, segsize);

    *result = pipeIn;

  } else {
    *result = &mInput;
  }

  mInputClosed = false;

  // Leave out SocketService's event handling
  NS_ADDREF(*result);
  return NS_OK;
}

NS_IMETHODIMP
nsCCNxTransport::OpenOutputStream(PRUint32 flags,
                                  PRUint32 segsize,
                                  PRUint32 segcount,
                                  nsIOutputStream **result) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCCNxTransport::Close(nsresult reason) {
  if (NS_SUCCEEDED(reason))
    reason = NS_BASE_STREAM_CLOSED;

  mInput.CloseWithStatus(reason);
  mInputClosed = true;
  return NS_OK;
}

NS_IMETHODIMP
nsCCNxTransport::SetEventSink(nsITransportEventSink *sink,
                              nsIEventTarget *target) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

void 
nsCCNxTransport::CCNX_MakeTemplate(int allow_stale) {
  mCCNxtmpl = ccn_charbuf_create();
  ccn_charbuf_append_tt(mCCNxtmpl, CCN_DTAG_Interest, CCN_DTAG);
  ccn_charbuf_append_tt(mCCNxtmpl, CCN_DTAG_Name, CCN_DTAG);
  ccn_charbuf_append_closer(mCCNxtmpl); /* </Name> */
  // XXX - use pubid if possible
  ccn_charbuf_append_tt(mCCNxtmpl, CCN_DTAG_MaxSuffixComponents, CCN_DTAG);
  ccnb_append_number(mCCNxtmpl, 1);
  ccn_charbuf_append_closer(mCCNxtmpl); /* </MaxSuffixComponents> */
  if (allow_stale) {
    ccn_charbuf_append_tt(mCCNxtmpl, CCN_DTAG_AnswerOriginKind, CCN_DTAG);
    ccnb_append_number(mCCNxtmpl, CCN_AOK_DEFAULT | CCN_AOK_STALE);
    ccn_charbuf_append_closer(mCCNxtmpl); /* </AnswerOriginKind> */
  }
  ccn_charbuf_append_closer(mCCNxtmpl); /* </Interest> */
}

void
nsCCNxTransport::CCNX_Close() {
  // XXX what if mCCNxstream has not been initialized yet?
  mCCNxstream = ccn_fetch_close(mCCNxstream);
  mCCNxfetch = ccn_fetch_destroy(mCCNxfetch);
  ccn_destroy(&mCCNx);
  ccn_charbuf_destroy(&mCCNxname);
  ccn_charbuf_destroy(&mCCNxtmpl);

  // TODO put the 'mInputClosed' into the right place
  mInputClosed = true;
}

struct ccn_fetch_stream*
nsCCNxTransport::CCNX_GetLocked() {
  // mCCNx is not available to the streams while it's not oneline
  if (!mCCNxonline)
    return nsnull;

  if (mCCNxstream)
    mCCNxref++;

  return mCCNxstream;
}

void
nsCCNxTransport::CCNX_ReleaseLocked(struct ccn_fetch_stream *ccnfs) {
  NS_ASSERTION(mCCNxstream == ccnfs, "wrong ndn");

  if (--mCCNxref == 0) {
    // close ndn here
    CCNX_Close();
  }
}

