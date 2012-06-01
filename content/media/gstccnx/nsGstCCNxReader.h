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
 *   Alessandro Decina <alessandro.d@gmail.com>
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

#ifndef nsGstCCNxReader_h__
#define nsGstCCNxReader_h__

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "nsBuiltinDecoderReader.h"

using namespace mozilla;

class nsMediaDecoder;
class nsTimeRanges;

class nsGstCCNxReader : public nsBuiltinDecoderReader
{
public:
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  nsGstCCNxReader(nsBuiltinDecoder* aDecoder);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  virtual ~nsGstCCNxReader();
  /**
   * TODO rewrite the Init function to use another gst pipeline
   */
  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  virtual nsresult ResetDecode();
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  virtual bool DecodeAudioData();
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                PRInt64 aTimeThreshold);
  /**
   * TODO rewrite the ReadMetadata function to fetch GstCaps using ccn_get
   */
  virtual nsresult ReadMetadata(nsVideoInfo* aInfo);
  /**
   * XXX let's deal with Seek after we got basic playback
   */
  virtual nsresult Seek(PRInt64 aTime,
                        PRInt64 aStartTime,
                        PRInt64 aEndTime,
                        PRInt64 aCurrentTime);
  /**
   * XXX let's deal with GetBuffered after we got basic playback
   * change the calculation of timestamp if necessary
   */
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);

  virtual bool HasAudio() {
    return mInfo.mHasAudio;
  }

  virtual bool HasVideo() {
    return mInfo.mHasVideo;
  }

private:

  /**
   * TODO Read data from CCNx buffer, need to modify code in ccnx protocol 
   */
  void ReadAndPushData(guint aLength);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  bool WaitForDecodedData(int *counter);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void NotifyBytesConsumed();
  /**
   * XXX let's deal with QueryQuration after we got basic playback
   */
  PRInt64 QueryDuration();

  /* Gst callbacks */

  /* Called on the source-setup signal emitted by playbin. Used to
   * configure appsrc .
   */
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  static void PlayBinSourceSetupCb(GstElement *aPlayBin,
                                   GstElement *aSource,
                                   gpointer aUserData);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void PlayBinSourceSetup(GstAppSrc *aSource);

  /* Called from appsrc when we need to read more data from the resource */
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  static void NeedDataCb(GstAppSrc *aSrc, guint aLength, gpointer aUserData);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void NeedData(GstAppSrc *aSrc, guint aLength);

  /* Called when appsrc has enough data and we can stop reading */
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  static void EnoughDataCb(GstAppSrc *aSrc, gpointer aUserData);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void EnoughData(GstAppSrc *aSrc);

  /* Called when a seek is issued on the pipeline */
  /**
   * XXX let's deal with Seek after we got basic playback
   */
  static gboolean SeekDataCb(GstAppSrc *aSrc,
                             guint64 aOffset,
                             gpointer aUserData);
  /**
   * XXX let's deal with Seek after we got basic playback
   */
  gboolean SeekData(GstAppSrc *aSrc, guint64 aOffset);

  /* Called when events reach the sinks. See inline comments */
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  static gboolean EventProbeCb(GstPad *aPad, GstEvent *aEvent, gpointer aUserData);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  gboolean EventProbe(GstPad *aPad, GstEvent *aEvent);

  /* Called when the pipeline is prerolled, that is when at start or after a
   * seek, the first audio and video buffers are queued in the sinks.
   */
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  static GstFlowReturn NewPrerollCb(GstAppSink *aSink, gpointer aUserData);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void VideoPreroll();
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void AudioPreroll();

  /* Called when buffers reach the sinks */
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  static GstFlowReturn NewBufferCb(GstAppSink *aSink, gpointer aUserData);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void NewVideoBuffer();
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void NewAudioBuffer();

  /* Called at end of stream, when decoding has finished */
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  static void EosCb(GstAppSink *aSink, gpointer aUserData);
  /**
   * XXX check whether it should be rewritten, I assume not
   */
  void Eos(GstAppSink *aSink);

  GstElement *mPlayBin;
  GstBus *mBus;
  GstAppSrc *mSource;
  /* video sink bin */
  GstElement *mVideoSink;
  /* the actual video app sink */
  GstAppSink *mVideoAppSink;
  /* audio sink bin */
  GstElement *mAudioSink;
  /* the actual audio app sink */
  GstAppSink *mAudioAppSink;
  GstVideoFormat mFormat;
  nsIntRect mPicture;
  int mVideoSinkBufferCount;
  int mAudioSinkBufferCount;
  GstAppSrcCallbacks mSrcCallbacks;
  GstAppSinkCallbacks mSinkCallbacks;
  /* monitor used to synchronize access to shared state between gstccnx
   * threads and other gecko threads */
  mozilla::ReentrantMonitor mGstThreadsMonitor;
  /* video and audio segments we use to convert absolute timestamps to [0,
   * stream_duration]. They're set when the pipeline is started or after a seek.
   * Concurrent access guarded with mGstThreadsMonitor.
   */
  GstSegment mVideoSegment;
  GstSegment mAudioSegment;
  /* bool used to signal when gst has detected the end of stream and
   * DecodeAudioData and DecodeVideoFrame should not expect any more data
   */
  bool mReachedEos;
  /* offset we've reached reading from the source */
  gint64 mByteOffset;
  /* the last offset we reported with NotifyBytesConsumed */
  gint64 mLastReportedByteOffset;
  int fpsNum;
  int fpsDen;
};

#endif
