/*
 * Copyright 2007, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "websettings"

#include <config.h>
#include <wtf/Platform.h>

#include "ApplicationCacheStorage.h"
#include "BitmapAllocatorAndroid.h"
#include "CachedResourceLoader.h"
#include "ChromiumIncludes.h"
#include "DatabaseTracker.h"
#include "Database.h"
#include "Document.h"
#include "EditorClientAndroid.h"
#include "FileSystem.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "GeolocationPermissions.h"
#include "GeolocationPositionCache.h"
#include "Page.h"
#include "PageCache.h"
#include "RenderTable.h"
#include "SQLiteFileSystem.h"
#include "Settings.h"
#include "WebCoreFrameBridge.h"
#include "WebCoreJni.h"
#include "LocalFileSystem.h"
// Samsung Change - HTML5 Web Notification	>>
#include "NotificationPresenterImpl.h"
// Samsung Change - HTML5 Web Notification	<<
#include "WorkerContextExecutionProxy.h"
#include "WebRequestContext.h"
#include "WebViewCore.h"
#include "WebSocket.h"

#include <JNIHelp.h>
#include <utils/misc.h>
#include <wtf/text/CString.h>

namespace android {

static const int permissionFlags660 = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

struct FieldIds {
    FieldIds(JNIEnv* env, jclass clazz) {
        mLayoutAlgorithm = env->GetFieldID(clazz, "mLayoutAlgorithm",
                "Landroid/webkit/WebSettings$LayoutAlgorithm;");
        mTextSize = env->GetFieldID(clazz, "mTextSize", "I");
        mStandardFontFamily = env->GetFieldID(clazz, "mStandardFontFamily",
                "Ljava/lang/String;");
        mFixedFontFamily = env->GetFieldID(clazz, "mFixedFontFamily",
                "Ljava/lang/String;");
        mSansSerifFontFamily = env->GetFieldID(clazz, "mSansSerifFontFamily",
                "Ljava/lang/String;");
        mSerifFontFamily = env->GetFieldID(clazz, "mSerifFontFamily",
                "Ljava/lang/String;");
        mCursiveFontFamily = env->GetFieldID(clazz, "mCursiveFontFamily",
                "Ljava/lang/String;");
        mFantasyFontFamily = env->GetFieldID(clazz, "mFantasyFontFamily",
                "Ljava/lang/String;");
        mDefaultTextEncoding = env->GetFieldID(clazz, "mDefaultTextEncoding",
                "Ljava/lang/String;");
// 4.2 Merge BEGIN <<
        mGetUserAgentString = env->GetMethodID(clazz, "getUserAgentString",
                "()Ljava/lang/String;");
        mGetAcceptLanguage = env->GetMethodID(clazz, "getAcceptLanguage", "()Ljava/lang/String;");
// 4.2 Merge END >>
        mMinimumFontSize = env->GetFieldID(clazz, "mMinimumFontSize", "I");
        mMinimumLogicalFontSize = env->GetFieldID(clazz, "mMinimumLogicalFontSize", "I");
        mDefaultFontSize = env->GetFieldID(clazz, "mDefaultFontSize", "I");
        mDefaultFixedFontSize = env->GetFieldID(clazz, "mDefaultFixedFontSize", "I");
        mLoadsImagesAutomatically = env->GetFieldID(clazz, "mLoadsImagesAutomatically", "Z");
#ifdef ANDROID_BLOCK_NETWORK_IMAGE
        mBlockNetworkImage = env->GetFieldID(clazz, "mBlockNetworkImage", "Z");
#endif
        mBlockNetworkLoads = env->GetFieldID(clazz, "mBlockNetworkLoads", "Z");
        mJavaScriptEnabled = env->GetFieldID(clazz, "mJavaScriptEnabled", "Z");
// SERI - WebGL >>
#if ENABLE(WEBGL)
        mWebGLEnabled = env->GetFieldID(clazz, "mWebGLEnabled", "Z");
#endif
// SERI - WebGL <<
        mAllowUniversalAccessFromFileURLs = env->GetFieldID(clazz, "mAllowUniversalAccessFromFileURLs", "Z");
        mAllowFileAccessFromFileURLs = env->GetFieldID(clazz, "mAllowFileAccessFromFileURLs", "Z");
        mPluginState = env->GetFieldID(clazz, "mPluginState",
                "Landroid/webkit/WebSettings$PluginState;");
#if ENABLE(DATABASE)
        mDatabaseEnabled = env->GetFieldID(clazz, "mDatabaseEnabled", "Z");
#endif
#if ENABLE(DOM_STORAGE)
        mDomStorageEnabled = env->GetFieldID(clazz, "mDomStorageEnabled", "Z");
#endif
#if ENABLE(DATABASE) || ENABLE(DOM_STORAGE)
        // The databases saved to disk for both the SQL and DOM Storage APIs are stored
        // in the same base directory.
        mDatabasePath = env->GetFieldID(clazz, "mDatabasePath", "Ljava/lang/String;");
        mDatabasePathHasBeenSet = env->GetFieldID(clazz, "mDatabasePathHasBeenSet", "Z");
#endif
#if ENABLE(FILE_SYSTEM)
        mFilesystemEnabled = env->GetFieldID(clazz, "mFilesystemEnabled", "Z");
        mFileSystemPath = env->GetFieldID(clazz, "mFileSystemPath", "Ljava/lang/String;");
        mFileSystemPathHasBeenSet = env->GetFieldID(clazz, "mFileSystemPathHasBeenSet", "Z");
        
#endif
// Samsung Change - HTML5 Web Notification	>>
#if ENABLE(NOTIFICATIONS)
        mWebnotificationEnabled = env->GetFieldID(clazz, "mWebnotificationEnabled", "Z");
        mWebnotificationDatabasePath = env->GetFieldID(clazz, "mWebnotificationDatabasePath", "Ljava/lang/String;");
        //mFileSystemPathHasBeenSet = env->GetFieldID(clazz, "mFileSystemPathHasBeenSet", "Z");
        mNotificationState = env->GetFieldID(clazz, "mNotificationState",
                "Landroid/webkit/WebSettings$NotificationState;");
        
#endif
// Samsung Change - HTML5 Web Notification	<<
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
        mAppCacheEnabled = env->GetFieldID(clazz, "mAppCacheEnabled", "Z");
        mAppCachePath = env->GetFieldID(clazz, "mAppCachePath", "Ljava/lang/String;");
        mAppCacheMaxSize = env->GetFieldID(clazz, "mAppCacheMaxSize", "J");
#endif
#if ENABLE(WORKERS)
        mWorkersEnabled = env->GetFieldID(clazz, "mWorkersEnabled", "Z");
#endif
        mGeolocationEnabled = env->GetFieldID(clazz, "mGeolocationEnabled", "Z");
        mGeolocationDatabasePath = env->GetFieldID(clazz, "mGeolocationDatabasePath", "Ljava/lang/String;");
        mXSSAuditorEnabled = env->GetFieldID(clazz, "mXSSAuditorEnabled", "Z");
#if ENABLE(LINK_PREFETCH)
        mLinkPrefetchEnabled = env->GetFieldID(clazz, "mLinkPrefetchEnabled", "Z");
#endif
        mJavaScriptCanOpenWindowsAutomatically = env->GetFieldID(clazz,
                "mJavaScriptCanOpenWindowsAutomatically", "Z");
        mUseWideViewport = env->GetFieldID(clazz, "mUseWideViewport", "Z");
        mSupportMultipleWindows = env->GetFieldID(clazz, "mSupportMultipleWindows", "Z");
        mShrinksStandaloneImagesToFit = env->GetFieldID(clazz, "mShrinksStandaloneImagesToFit", "Z");
        mMaximumDecodedImageSize = env->GetFieldID(clazz, "mMaximumDecodedImageSize", "J");
        mPrivateBrowsingEnabled = env->GetFieldID(clazz, "mPrivateBrowsingEnabled", "Z");
        mSyntheticLinksEnabled = env->GetFieldID(clazz, "mSyntheticLinksEnabled", "Z");
        mUseDoubleTree = env->GetFieldID(clazz, "mUseDoubleTree", "Z");
        mPageCacheCapacity = env->GetFieldID(clazz, "mPageCacheCapacity", "I");
//SAMSUNG ADVANCED TEXT SELECTION - BEGIN 
        mAdvanceTextSelection =  env->GetFieldID(clazz, "mAdvanceTextSelection", "Z");
        mAdvanceSelectionBgColor =  env->GetFieldID(clazz, "mAdvanceSelectionBgColor", "J");
//SAMSUNG ADVANCED TEXT SELECTION - END
//SISO_HTMLComposer Start
	mEditableSupport =  env->GetFieldID(clazz, "mEditableSupport", "Z");	
        mDisableAnimation =  env->GetFieldID(clazz, "mDisableAnimation", "Z");	
        mHighResolutionDevice = env->GetFieldID(clazz, "mHighResolutionDevice", "Z");
//SISO_HTMLComposer End	
#if ENABLE(WEB_AUTOFILL)
        mAutoFillEnabled = env->GetFieldID(clazz, "mAutoFillEnabled", "Z");
        mAutoFillProfile = env->GetFieldID(clazz, "mAutoFillProfile", "Landroid/webkit/WebSettingsClassic$AutoFillProfile;");
        jclass autoFillProfileClass = env->FindClass("android/webkit/WebSettingsClassic$AutoFillProfile");
        mAutoFillProfileFullName = env->GetFieldID(autoFillProfileClass, "mFullName", "Ljava/lang/String;");
        mAutoFillProfileEmailAddress = env->GetFieldID(autoFillProfileClass, "mEmailAddress", "Ljava/lang/String;");
        mAutoFillProfileCompanyName = env->GetFieldID(autoFillProfileClass, "mCompanyName", "Ljava/lang/String;");
        mAutoFillProfileAddressLine1 = env->GetFieldID(autoFillProfileClass, "mAddressLine1", "Ljava/lang/String;");
        mAutoFillProfileAddressLine2 = env->GetFieldID(autoFillProfileClass, "mAddressLine2", "Ljava/lang/String;");
        mAutoFillProfileCity = env->GetFieldID(autoFillProfileClass, "mCity", "Ljava/lang/String;");
        mAutoFillProfileState = env->GetFieldID(autoFillProfileClass, "mState", "Ljava/lang/String;");
        mAutoFillProfileZipCode = env->GetFieldID(autoFillProfileClass, "mZipCode", "Ljava/lang/String;");
        mAutoFillProfileCountry = env->GetFieldID(autoFillProfileClass, "mCountry", "Ljava/lang/String;");
        mAutoFillProfilePhoneNumber = env->GetFieldID(autoFillProfileClass, "mPhoneNumber", "Ljava/lang/String;");
        env->DeleteLocalRef(autoFillProfileClass);
#endif
        mOverrideCacheMode = env->GetFieldID(clazz, "mOverrideCacheMode", "I");
        mPasswordEchoEnabled = env->GetFieldID(clazz, "mPasswordEchoEnabled", "Z");
        mMediaPlaybackRequiresUserGesture = env->GetFieldID(clazz, "mMediaPlaybackRequiresUserGesture", "Z");//4.2 Merge
//SAMSUNG CHANGES >>> SPELLCHECK(sataya.m@samsung.com)
#if ENABLE(SPELLCHECK)
		mIsSpellCheckEnabled = env->GetFieldID(clazz, "mIsSpellCheckEnabled", "Z");
#endif
//SAMSUNG CHANGES <<<

		//SAMSUNG_CHANGES >>
		mSupportRssSniffing = env->GetFieldID(clazz, "mSupportRssSniffing", "Z");
		mIsBrowserApp = env->GetFieldID(clazz, "mIsBrowserApp", "Z");
		mIsSkiaPictureEnabled = env->GetFieldID(clazz, "mIsSkiaPictureEnabled", "Z");
        //SAMSUNG_CHANGES <<

	    //SAMSUNG CHANGES WOFF <<
	    mWOFFEnabled = env->GetFieldID(clazz, "mWOFFEnabled", "Z");
	    //SAMSUNG CHANGES WOFF >>

        // SAMSUNG CHANGES IMIDEO SERVICE >>
        mEnableImideo = env->GetFieldID(clazz, "mEnableImideo","I");
        // SAMSUNG CHANGES IMIDEO SERVICE <<

        ALOG_ASSERT(mLayoutAlgorithm, "Could not find field mLayoutAlgorithm");
        ALOG_ASSERT(mTextSize, "Could not find field mTextSize");
        ALOG_ASSERT(mStandardFontFamily, "Could not find field mStandardFontFamily");
        ALOG_ASSERT(mFixedFontFamily, "Could not find field mFixedFontFamily");
        ALOG_ASSERT(mSansSerifFontFamily, "Could not find field mSansSerifFontFamily");
        ALOG_ASSERT(mSerifFontFamily, "Could not find field mSerifFontFamily");
        ALOG_ASSERT(mCursiveFontFamily, "Could not find field mCursiveFontFamily");
        ALOG_ASSERT(mFantasyFontFamily, "Could not find field mFantasyFontFamily");
        ALOG_ASSERT(mDefaultTextEncoding, "Could not find field mDefaultTextEncoding");
// 4.2 Merge BEGIN <<
        ALOG_ASSERT(mGetUserAgentString, "Could not find method getUserAgentString");
        ALOG_ASSERT(mGetAcceptLanguage, "Could not find method getAcceptLanguage");
// 4.2 Merge END >>
        ALOG_ASSERT(mMinimumFontSize, "Could not find field mMinimumFontSize");
        ALOG_ASSERT(mMinimumLogicalFontSize, "Could not find field mMinimumLogicalFontSize");
        ALOG_ASSERT(mDefaultFontSize, "Could not find field mDefaultFontSize");
        ALOG_ASSERT(mDefaultFixedFontSize, "Could not find field mDefaultFixedFontSize");
        ALOG_ASSERT(mLoadsImagesAutomatically, "Could not find field mLoadsImagesAutomatically");
#ifdef ANDROID_BLOCK_NETWORK_IMAGE
        ALOG_ASSERT(mBlockNetworkImage, "Could not find field mBlockNetworkImage");
#endif
        ALOG_ASSERT(mBlockNetworkLoads, "Could not find field mBlockNetworkLoads");
        ALOG_ASSERT(mJavaScriptEnabled, "Could not find field mJavaScriptEnabled");
// SERI - WebGL >>
#if ENABLE(WEBGL)
        ALOG_ASSERT(mWebGLEnabled, "Could not find field mWebGLEnabled");
#endif
// SERI - WebGL <<
        ALOG_ASSERT(mAllowUniversalAccessFromFileURLs,
                    "Could not find field mAllowUniversalAccessFromFileURLs");
        ALOG_ASSERT(mAllowFileAccessFromFileURLs,
                    "Could not find field mAllowFileAccessFromFileURLs");
        ALOG_ASSERT(mPluginState, "Could not find field mPluginState");
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
        ALOG_ASSERT(mAppCacheEnabled, "Could not find field mAppCacheEnabled");
        ALOG_ASSERT(mAppCachePath, "Could not find field mAppCachePath");
        ALOG_ASSERT(mAppCacheMaxSize, "Could not find field mAppCacheMaxSize");
#endif
#if ENABLE(WORKERS)
        ALOG_ASSERT(mWorkersEnabled, "Could not find field mWorkersEnabled");
#endif
        ALOG_ASSERT(mJavaScriptCanOpenWindowsAutomatically,
                "Could not find field mJavaScriptCanOpenWindowsAutomatically");
        ALOG_ASSERT(mUseWideViewport, "Could not find field mUseWideViewport");
        ALOG_ASSERT(mSupportMultipleWindows, "Could not find field mSupportMultipleWindows");
        ALOG_ASSERT(mShrinksStandaloneImagesToFit, "Could not find field mShrinksStandaloneImagesToFit");
        ALOG_ASSERT(mMaximumDecodedImageSize, "Could not find field mMaximumDecodedImageSize");
        ALOG_ASSERT(mUseDoubleTree, "Could not find field mUseDoubleTree");
        ALOG_ASSERT(mPageCacheCapacity, "Could not find field mPageCacheCapacity");
        ALOG_ASSERT(mPasswordEchoEnabled, "Could not find field mPasswordEchoEnabled");
        ALOG_ASSERT(mMediaPlaybackRequiresUserGesture, "Could not find field mMediaPlaybackRequiresUserGesture");//4.2 Merge
//SAMSUNG ADVANCED TEXT SELECTION - BEGIN
        ALOG_ASSERT(mAdvanceTextSelection, "Could not find field mAdvanceTextSelection");
        ALOG_ASSERT(mAdvanceSelectionBgColor, "Could not find field mAdvanceSelectionBgColor");
//SAMSUNG ADVANCED TEXT SELECTION - END
//SAMSUNG CHANGES >>> SPELLCHECK(sataya.m@samsung.com)
#if ENABLE(SPELLCHECK)
		ALOG_ASSERT(mIsSpellCheckEnabled, "Could not find field mIsSpellCheckEnabled");
#endif
//SAMSUNG CHANGES <<<	

		//SAMSUNG_CHANGES >>
		ALOG_ASSERT(mSupportRssSniffing, "Could not find field mSupportRssSniffing");
		ALOG_ASSERT(mIsBrowserApp, "Could not find field mIsBrowserApp");
		ALOG_ASSERT(mIsSkiaPictureEnabled, "Could not find field mIsSkiaPictureEnabled");
		//SAMSUNG_CHANGES >>
	//SAMSUNG CHANGES WOFF <<
	ALOG_ASSERT(mWOFFEnabled, "Could not find field mWOFFEnabled");
	//SAMSUNG CHANGES WOFF >>

        jclass enumClass = env->FindClass("java/lang/Enum");
        ALOG_ASSERT(enumClass, "Could not find Enum class!");
        mOrdinal = env->GetMethodID(enumClass, "ordinal", "()I");
        ALOG_ASSERT(mOrdinal, "Could not find method ordinal");
        env->DeleteLocalRef(enumClass);

        // SAMSUNG CHANGES IMIDEO SERVICE >>
        ALOG_ASSERT(mEnableImideo, "Could not find field mEnableImideo");
        // SAMSUNG CHANGES IMIDEO SERVICE <<
    }

    // Field ids
    jfieldID mLayoutAlgorithm;
    jfieldID mTextSize;
    jfieldID mStandardFontFamily;
    jfieldID mFixedFontFamily;
    jfieldID mSansSerifFontFamily;
    jfieldID mSerifFontFamily;
    jfieldID mCursiveFontFamily;
    jfieldID mFantasyFontFamily;
    jfieldID mDefaultTextEncoding;
// 4.2 Merge BEGIN <<
    jmethodID mGetUserAgentString;
    jmethodID mGetAcceptLanguage;
// 4.2 Merge END >>
    jfieldID mMinimumFontSize;
    jfieldID mMinimumLogicalFontSize;
    jfieldID mDefaultFontSize;
    jfieldID mDefaultFixedFontSize;
    jfieldID mLoadsImagesAutomatically;
#ifdef ANDROID_BLOCK_NETWORK_IMAGE
    jfieldID mBlockNetworkImage;
#endif
    jfieldID mBlockNetworkLoads;
    jfieldID mJavaScriptEnabled;
// SERI - WebGL >>
#if ENABLE(WEBGL)
    jfieldID mWebGLEnabled;
#endif
// SERI - WebGL <<
    jfieldID mAllowUniversalAccessFromFileURLs;
    jfieldID mAllowFileAccessFromFileURLs;
    jfieldID mPluginState;
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    jfieldID mAppCacheEnabled;
    jfieldID mAppCachePath;
    jfieldID mAppCacheMaxSize;
#endif
#if ENABLE(WORKERS)
    jfieldID mWorkersEnabled;
#endif
    jfieldID mJavaScriptCanOpenWindowsAutomatically;
    jfieldID mUseWideViewport;
    jfieldID mSupportMultipleWindows;
    jfieldID mShrinksStandaloneImagesToFit;
    jfieldID mMaximumDecodedImageSize;
    jfieldID mPrivateBrowsingEnabled;
    jfieldID mSyntheticLinksEnabled;
    jfieldID mUseDoubleTree;
    jfieldID mPageCacheCapacity;
    // Ordinal() method and value field for enums
    jmethodID mOrdinal;
    jfieldID  mTextSizeValue;

#if ENABLE(DATABASE)
    jfieldID mDatabaseEnabled;
#endif
#if ENABLE(DOM_STORAGE)
    jfieldID mDomStorageEnabled;
#endif
    jfieldID mGeolocationEnabled;
    jfieldID mGeolocationDatabasePath;
    jfieldID mXSSAuditorEnabled;
#if ENABLE(LINK_PREFETCH)
    jfieldID mLinkPrefetchEnabled;
#endif
#if ENABLE(DATABASE) || ENABLE(DOM_STORAGE)
    jfieldID mDatabasePath;
    jfieldID mDatabasePathHasBeenSet;
#endif
#if ENABLE(FILE_SYSTEM)
    jfieldID mFilesystemEnabled;
    jfieldID mFileSystemPath;
    jfieldID mFileSystemPathHasBeenSet;
#endif
// Samsung Change - HTML5 Web Notification	>>
#if ENABLE(NOTIFICATIONS)
    jfieldID mWebnotificationDatabasePath;
    jfieldID mWebnotificationEnabled;
    jfieldID mNotificationState;
#endif
// Samsung Change - HTML5 Web Notification	<<
#if ENABLE(WEB_AUTOFILL)
    jfieldID mAutoFillEnabled;
    jfieldID mAutoFillProfile;
    jfieldID mAutoFillProfileFullName;
    jfieldID mAutoFillProfileEmailAddress;
    jfieldID mAutoFillProfileCompanyName;
    jfieldID mAutoFillProfileAddressLine1;
    jfieldID mAutoFillProfileAddressLine2;
    jfieldID mAutoFillProfileCity;
    jfieldID mAutoFillProfileState;
    jfieldID mAutoFillProfileZipCode;
    jfieldID mAutoFillProfileCountry;
    jfieldID mAutoFillProfilePhoneNumber;
#endif
//SAMSUNG ADVANCED TEXT SELECTION - BEGIN
    jfieldID mAdvanceTextSelection; 
    jfieldID mAdvanceSelectionBgColor;
//SAMSUNG ADVANCED TEXT SELECTION - END
    jfieldID mOverrideCacheMode;
//SISO_HTMLComposer Start
    jfieldID mEditableSupport;			
    jfieldID mDisableAnimation;		
    jfieldID mHighResolutionDevice;
//SISO_HTMLComposer End
	
//SAMSUNG CHANGES >>> SPELLCHECK(sataya.m@samsung.com)
#if ENABLE(SPELLCHECK)
	jfieldID mIsSpellCheckEnabled;
#endif
//SAMSUNG CHANGES <<<	

	//SAMSUNG_CHANGES >>
	jfieldID mSupportRssSniffing;
	jfieldID mIsBrowserApp;
	jfieldID mIsSkiaPictureEnabled;
	//SAMSUNG_CHANGES <<
    
    jfieldID mPasswordEchoEnabled;
    jfieldID mMediaPlaybackRequiresUserGesture;//4.2 Merge
    //SAMSUNG CHANGES WOFF <<
    jfieldID mWOFFEnabled;
    //SAMSUNG CHANGES WOFF <<	
    // SAMSUNG CHANGES IMIDEO SERVICE >>
    jfieldID mEnableImideo;
    // SAMSUNG CHANGES IMIDEO SERVICE >>
};

static struct FieldIds* gFieldIds;

// Note: This is moved from the old FrameAndroid.cpp
static void recursiveCleanupForFullLayout(WebCore::RenderObject* obj)
{
    obj->setNeedsLayout(true, false);
#ifdef ANDROID_LAYOUT
    if (obj->isTable())
        (static_cast<WebCore::RenderTable *>(obj))->clearSingleColumn();
#endif
    for (WebCore::RenderObject* n = obj->firstChild(); n; n = n->nextSibling())
        recursiveCleanupForFullLayout(n);
}

#if ENABLE(WEB_AUTOFILL)
inline string16 getStringFieldAsString16(JNIEnv* env, jobject autoFillProfile, jfieldID fieldId)
{
    jstring str = static_cast<jstring>(env->GetObjectField(autoFillProfile, fieldId));
    return str ? jstringToString16(env, str) : string16();
}

void syncAutoFillProfile(JNIEnv* env, jobject autoFillProfile, WebAutofill* webAutofill)
{
    string16 fullName = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileFullName);
    string16 emailAddress = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileEmailAddress);
    string16 companyName = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileCompanyName);
    string16 addressLine1 = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileAddressLine1);
    string16 addressLine2 = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileAddressLine2);
    string16 city = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileCity);
    string16 state = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileState);
    string16 zipCode = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileZipCode);
    string16 country = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfileCountry);
    string16 phoneNumber = getStringFieldAsString16(env, autoFillProfile, gFieldIds->mAutoFillProfilePhoneNumber);

    webAutofill->setProfile(fullName, emailAddress, companyName, addressLine1, addressLine2, city, state, zipCode, country, phoneNumber);
}
#endif

class WebSettings {
public:
    static void Sync(JNIEnv* env, jobject obj, jint frame)
    {
        WebCore::Frame* pFrame = (WebCore::Frame*)frame;
        ALOG_ASSERT(pFrame, "%s must take a valid frame pointer!", __FUNCTION__);
        WebCore::Settings* s = pFrame->settings();
        if (!s)
            return;
        WebCore::CachedResourceLoader* cachedResourceLoader = pFrame->document()->cachedResourceLoader();

#ifdef ANDROID_LAYOUT
        jobject layout = env->GetObjectField(obj, gFieldIds->mLayoutAlgorithm);
        WebCore::Settings::LayoutAlgorithm l = (WebCore::Settings::LayoutAlgorithm)
                env->CallIntMethod(layout, gFieldIds->mOrdinal);
        if (s->layoutAlgorithm() != l) {
            s->setLayoutAlgorithm(l);
            if (pFrame->document()) {
                pFrame->document()->styleSelectorChanged(WebCore::RecalcStyleImmediately);
                if (pFrame->document()->renderer()) {
                    recursiveCleanupForFullLayout(pFrame->document()->renderer());
                    ALOG_ASSERT(pFrame->view(), "No view for this frame when trying to relayout");
                    pFrame->view()->layout();
                    // FIXME: This call used to scroll the page to put the focus into view.
                    // It worked on the WebViewCore, but now scrolling is done outside of the
                    // WebViewCore, on the UI side, so there needs to be a new way to do this.
                    //pFrame->makeFocusVisible();
                }
            }
        }
#endif
        jint textSize = env->GetIntField(obj, gFieldIds->mTextSize);
        float zoomFactor = textSize / 100.0f;
        if (pFrame->textZoomFactor() != zoomFactor)
            pFrame->setTextZoomFactor(zoomFactor);

        jstring str = (jstring)env->GetObjectField(obj, gFieldIds->mStandardFontFamily);
        s->setStandardFontFamily(jstringToWtfString(env, str));

        str = (jstring)env->GetObjectField(obj, gFieldIds->mFixedFontFamily);
        s->setFixedFontFamily(jstringToWtfString(env, str));

        str = (jstring)env->GetObjectField(obj, gFieldIds->mSansSerifFontFamily);
        s->setSansSerifFontFamily(jstringToWtfString(env, str));

        str = (jstring)env->GetObjectField(obj, gFieldIds->mSerifFontFamily);
        s->setSerifFontFamily(jstringToWtfString(env, str));

        str = (jstring)env->GetObjectField(obj, gFieldIds->mCursiveFontFamily);
        s->setCursiveFontFamily(jstringToWtfString(env, str));

        str = (jstring)env->GetObjectField(obj, gFieldIds->mFantasyFontFamily);
        s->setFantasyFontFamily(jstringToWtfString(env, str));

        str = (jstring)env->GetObjectField(obj, gFieldIds->mDefaultTextEncoding);
//SAMSUNG Change >>
        String defaultEncoding = jstringToWtfString(env, str);
        if (defaultEncoding == "AutoDetect") {
            s->setUsesEncodingDetector(true);
        } else {
            s->setUsesEncodingDetector(false);
            s->setDefaultTextEncodingName(defaultEncoding);
        }
//SAMSUNG Change <<
        //s->setDefaultTextEncodingName(jstringToWtfString(env, str));

        str = (jstring)env->CallObjectMethod(obj, gFieldIds->mGetUserAgentString);//4.2 Merge
        WebFrame::getWebFrame(pFrame)->setUserAgent(jstringToWtfString(env, str));
        WebViewCore::getWebViewCore(pFrame->view())->setWebRequestContextUserAgent();

        jint cacheMode = env->GetIntField(obj, gFieldIds->mOverrideCacheMode);
        WebViewCore::getWebViewCore(pFrame->view())->setWebRequestContextCacheMode(cacheMode);

        str = (jstring)env->CallObjectMethod(obj, gFieldIds->mGetAcceptLanguage);//4.2 Merge
        WebRequestContext::setAcceptLanguage(jstringToWtfString(env, str));

        jint size = env->GetIntField(obj, gFieldIds->mMinimumFontSize);
        s->setMinimumFontSize(size);

        size = env->GetIntField(obj, gFieldIds->mMinimumLogicalFontSize);
        s->setMinimumLogicalFontSize(size);

        size = env->GetIntField(obj, gFieldIds->mDefaultFontSize);
        s->setDefaultFontSize(size);

        size = env->GetIntField(obj, gFieldIds->mDefaultFixedFontSize);
        s->setDefaultFixedFontSize(size);

        jboolean flag = env->GetBooleanField(obj, gFieldIds->mLoadsImagesAutomatically);
        s->setLoadsImagesAutomatically(flag);
        if (flag)
            cachedResourceLoader->setAutoLoadImages(true);

#ifdef ANDROID_BLOCK_NETWORK_IMAGE
        flag = env->GetBooleanField(obj, gFieldIds->mBlockNetworkImage);
        s->setBlockNetworkImage(flag);
        if(!flag)
            cachedResourceLoader->setBlockNetworkImage(false);
#endif
        flag = env->GetBooleanField(obj, gFieldIds->mBlockNetworkLoads);
        WebFrame* webFrame = WebFrame::getWebFrame(pFrame);
        webFrame->setBlockNetworkLoads(flag);

        flag = env->GetBooleanField(obj, gFieldIds->mJavaScriptEnabled);
        s->setJavaScriptEnabled(flag);

// SERI - WebGL >>
#if ENABLE(WEBGL)
        flag = env->GetBooleanField(obj, gFieldIds->mWebGLEnabled);
        s->setWebGLEnabled(flag);
#endif
// SERI - WebGL <<

        flag = env->GetBooleanField(obj, gFieldIds->mAllowUniversalAccessFromFileURLs);
        s->setAllowUniversalAccessFromFileURLs(flag);

        flag = env->GetBooleanField(obj, gFieldIds->mAllowFileAccessFromFileURLs);
        s->setAllowFileAccessFromFileURLs(flag);

        // Hyperlink auditing (the ping attribute) has similar privacy
        // considerations as does the running of JavaScript, so to keep the UI
        // simpler, we leverage the same setting.
        s->setHyperlinkAuditingEnabled(flag);

        // ON = 0
        // ON_DEMAND = 1
        // OFF = 2
        jobject pluginState = env->GetObjectField(obj, gFieldIds->mPluginState);
        int state = env->CallIntMethod(pluginState, gFieldIds->mOrdinal);
        s->setPluginsEnabled(state < 2);
#ifdef ANDROID_PLUGINS
        s->setPluginsOnDemand(state == 1);
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
        // We only enable AppCache if it's been enabled with a call to
        // setAppCacheEnabled() and if a valid path has been supplied to
        // setAppCachePath(). Note that the path is applied to all WebViews
        // whereas enabling is applied per WebView.

        // WebCore asserts that the path is only set once. Since the path is
        // shared between WebViews, we can't do the required checks to guard
        // against this in the Java WebSettings.
        bool isPathValid = false;
        if (cacheStorage().cacheDirectory().isNull()) {
            str = static_cast<jstring>(env->GetObjectField(obj, gFieldIds->mAppCachePath));
            // Check for non-null string as an optimization, as this is the common case.
            if (str) {
                String path = jstringToWtfString(env, str);
                ALOG_ASSERT(!path.empty(), "Java side should never send empty string for AppCache path");
                // This database is created on the first load. If the file
                // doesn't exist, we create it and set its permissions. The
                // filename must match that in ApplicationCacheStorage.cpp.
                String filename = pathByAppendingComponent(path, "ApplicationCache.db");
                int fd = open(filename.utf8().data(), O_CREAT, permissionFlags660);
                if (fd >= 0) {
                    close(fd);
                    cacheStorage().setCacheDirectory(path);
                    isPathValid = true;
              }
            }
        } else
            isPathValid = true;

        flag = env->GetBooleanField(obj, gFieldIds->mAppCacheEnabled);
        s->setOfflineWebApplicationCacheEnabled(flag && isPathValid);

        jlong maxsize = env->GetLongField(obj, gFieldIds->mAppCacheMaxSize);
        cacheStorage().setMaximumSize(maxsize);
#endif

        flag = env->GetBooleanField(obj, gFieldIds->mJavaScriptCanOpenWindowsAutomatically);
        s->setJavaScriptCanOpenWindowsAutomatically(flag);

#ifdef ANDROID_LAYOUT
        flag = env->GetBooleanField(obj, gFieldIds->mUseWideViewport);
        s->setUseWideViewport(flag);
#endif

#ifdef ANDROID_MULTIPLE_WINDOWS
        flag = env->GetBooleanField(obj, gFieldIds->mSupportMultipleWindows);
        s->setSupportMultipleWindows(flag);
#endif
        flag = env->GetBooleanField(obj, gFieldIds->mShrinksStandaloneImagesToFit);
        s->setShrinksStandaloneImagesToFit(flag);
        jlong maxImage = env->GetLongField(obj, gFieldIds->mMaximumDecodedImageSize);
        // Since in ImageSourceAndroid.cpp, the image will always not exceed
        // MAX_SIZE_BEFORE_SUBSAMPLE, there's no need to pass the max value to
        // WebCore, which checks (image_width * image_height * 4) as an
        // estimation against the max value, which is done in CachedImage.cpp.
        // And there're cases where the decoded image size will not
        // exceed the max, but the WebCore estimation will.  So the following
        // code is commented out to fix those cases.
        // if (maxImage == 0)
        //    maxImage = computeMaxBitmapSizeForCache();
        s->setMaximumDecodedImageSize(maxImage);

        flag = env->GetBooleanField(obj, gFieldIds->mPrivateBrowsingEnabled);
        s->setPrivateBrowsingEnabled(flag);

        flag = env->GetBooleanField(obj, gFieldIds->mSyntheticLinksEnabled);
        s->setDefaultFormatDetection(flag);
        s->setFormatDetectionAddress(flag);
        s->setFormatDetectionEmail(flag);
        s->setFormatDetectionTelephone(flag);
#if ENABLE(DATABASE)
        flag = env->GetBooleanField(obj, gFieldIds->mDatabaseEnabled);
        WebCore::Database::setIsAvailable(flag);

        flag = env->GetBooleanField(obj, gFieldIds->mDatabasePathHasBeenSet);
        if (flag) {
            // If the user has set the database path, sync it to the DatabaseTracker.
            str = (jstring)env->GetObjectField(obj, gFieldIds->mDatabasePath);
            if (str) {
                String path = jstringToWtfString(env, str);
                DatabaseTracker::tracker().setDatabaseDirectoryPath(path);
                // This database is created when the first HTML5 Database object is
                // instantiated. If the file doesn't exist, we create it and set its
                // permissions. The filename must match that in
                // DatabaseTracker.cpp.
                String filename = SQLiteFileSystem::appendDatabaseFileNameToPath(path, "Databases.db");
                int fd = open(filename.utf8().data(), O_CREAT | O_EXCL, permissionFlags660);
                if (fd >= 0)
                    close(fd);
            }
        }
#endif
#if ENABLE(FILE_SYSTEM)
        flag = env->GetBooleanField(obj, gFieldIds->mFilesystemEnabled);
        flag = env->GetBooleanField(obj, gFieldIds->mFileSystemPathHasBeenSet);
        if (flag) {
            // If the user has set the filesystem path, sync it to the LocalFileSystem.
            str = (jstring)env->GetObjectField(obj, gFieldIds->mFileSystemPath);
            if (str) {
                String path = jstringToWtfString(env, str);
                LocalFileSystem::localFileSystem().initializeLocalFileSystem(path);
            }
        }
#endif

// Samsung Change - HTML5 Web Notification	>>
#if ENABLE(NOTIFICATIONS)
	flag = env->GetBooleanField(obj, gFieldIds->mWebnotificationEnabled);
	//flag = env->GetBooleanField(obj, gFieldIds->mFileSystemPathHasBeenSet);
	//if (flag) {
	// If the user has set the Web notification path, sync it to the NotificationPresenterImpl.
	str = (jstring)env->GetObjectField(obj, gFieldIds->mWebnotificationDatabasePath);
	if (str) {
		String path = jstringToWtfString(env, str);
		NotificationPresenterImpl::setDatabasePath(path);
	}

	// ALWAYS ON = 0
        // ON_DEMAND = 1
        // OFF = 2
        jobject notificationState = env->GetObjectField(obj, gFieldIds->mNotificationState);
        int notifystate = env->CallIntMethod(notificationState, gFieldIds->mOrdinal);
	NotificationPresenterImpl::setSettingsValue(notifystate);
        //s->setPluginsEnabled(state < 2);

	   
        //}
#endif
// Samsung Change - HTML5 Web Notification	<<

#if ENABLE(DOM_STORAGE)
        flag = env->GetBooleanField(obj, gFieldIds->mDomStorageEnabled);
        s->setLocalStorageEnabled(flag);
        str = (jstring)env->GetObjectField(obj, gFieldIds->mDatabasePath);
        if (str) {
            WTF::String localStorageDatabasePath = jstringToWtfString(env,str);
            if (localStorageDatabasePath.length()) {
                localStorageDatabasePath = WebCore::pathByAppendingComponent(
                        localStorageDatabasePath, "localstorage");
                // We need 770 for folders
                mkdir(localStorageDatabasePath.utf8().data(),
                        permissionFlags660 | S_IXUSR | S_IXGRP);
                s->setLocalStorageDatabasePath(localStorageDatabasePath);
            }
        }
#endif
//SISO_HTMLComposer Start
	flag = env->GetBooleanField(obj, gFieldIds->mEditableSupport);
	if(flag)        
		s->setEditableLinkBehavior(WebCore::EditableLinkNeverLive);
		
	s->setEditableSupportEnabled(flag);		

    flag = env->GetBooleanField(obj, gFieldIds->mDisableAnimation);	
    s->setDisableAnimation(flag);

    flag = env->GetBooleanField(obj, gFieldIds->mHighResolutionDevice);
    s->setHighResolutionDevice(flag);
//SISO_HTMLComposer End	

//SAMSUNG ADVANCED TEXT SELECTION - BEGIN
        flag = env->GetBooleanField(obj, gFieldIds->mAdvanceTextSelection);
        s->setAdvancedSelectionEnabled(flag);
        jlong color = env->GetLongField(obj, gFieldIds->mAdvanceSelectionBgColor);
        if (-1 != color) {
            int r = ((color & 0x00FF0000) >> 16);
            int g = ((color & 0x0000FF00) >> 8);
            int b = (color & 0x000000FF);
            s->setAdvancedSelectionBgColor(r, g, b);
        }
//SAMSUNG ADVANCED TEXT SELECTION - END

//SAMSUNG CHANGES >>> SPELLCHECK(sataya.m@samsung.com)
#if ENABLE(SPELLCHECK)
		flag = env->GetBooleanField(obj, gFieldIds->mIsSpellCheckEnabled);
	s->setIsContinousSpellCheck(flag);
#endif
//SAMSUNG CHANGES <<<	
    
		//SAMSUNG_CHANGES >>
		flag = env->GetBooleanField(obj, gFieldIds->mSupportRssSniffing);
        s->setRssSniffingEnabled(flag);
		flag = env->GetBooleanField(obj, gFieldIds->mIsBrowserApp);
        s->setIsBrowserApp(flag);        
		flag = env->GetBooleanField(obj, gFieldIds->mIsSkiaPictureEnabled);
        s->setSkiaPictureEnabled(flag);		
		//SAMSUNG_CHANGES <<

        flag = env->GetBooleanField(obj, gFieldIds->mGeolocationEnabled);
        GeolocationPermissions::setAlwaysDeny(!flag);
        str = (jstring)env->GetObjectField(obj, gFieldIds->mGeolocationDatabasePath);
        if (str) {
            String path = jstringToWtfString(env, str);
            GeolocationPermissions::setDatabasePath(path);
            GeolocationPositionCache::instance()->setDatabasePath(path);
            // This database is created when the first Geolocation object is
            // instantiated. If the file doesn't exist, we create it and set its
            // permissions. The filename must match that in
            // GeolocationPositionCache.cpp.
            String filename = SQLiteFileSystem::appendDatabaseFileNameToPath(path, "CachedGeoposition.db");
            int fd = open(filename.utf8().data(), O_CREAT | O_EXCL, permissionFlags660);
            if (fd >= 0)
                close(fd);
        }

        flag = env->GetBooleanField(obj, gFieldIds->mXSSAuditorEnabled);
        s->setXSSAuditorEnabled(flag);

#if ENABLE(LINK_PREFETCH)
        flag = env->GetBooleanField(obj, gFieldIds->mLinkPrefetchEnabled);
        s->setLinkPrefetchEnabled(flag);
#endif

        size = env->GetIntField(obj, gFieldIds->mPageCacheCapacity);
        if (size > 0) {
            s->setUsesPageCache(true);
            WebCore::pageCache()->setCapacity(size);
        } else
            s->setUsesPageCache(false);

#if ENABLE(WEB_AUTOFILL)
        flag = env->GetBooleanField(obj, gFieldIds->mAutoFillEnabled);
        // TODO: This updates the Settings WebCore side with the user's
        // preference for autofill and will stop WebCore making requests
        // into the chromium autofill code. That code in Chromium also has
        // a notion of being enabled/disabled that gets read from the users
        // preferences. At the moment, it's hardcoded to true on Android
        // (see chrome/browser/autofill/autofill_manager.cc:405). This
        // setting should probably be synced into Chromium also.

        s->setAutoFillEnabled(flag);

        if (flag) {
            EditorClientAndroid* editorC = static_cast<EditorClientAndroid*>(pFrame->page()->editorClient());
            WebAutofill* webAutofill = editorC->getAutofill();
            // Set the active AutofillProfile data.
            jobject autoFillProfile = env->GetObjectField(obj, gFieldIds->mAutoFillProfile);
            if (autoFillProfile)
                syncAutoFillProfile(env, autoFillProfile, webAutofill);
            else {
                // The autofill profile is null. We need to tell Chromium about this because
                // this may be because the user just deleted their profile but left the
                // autofill feature setting enabled.
                webAutofill->clearProfiles();
            }
        }
#endif

//SAMSUNG CHANGES WEBSOCKET <<
#if ENABLE(WEB_SOCKETS)		
		WebCore::WebSocket::setIsAvailable(true);
#endif
//SAMSUNG CHANGES WEBSOCKET >>
//SAMSUNG CHANGES WOFF <<
	 flag = env->GetBooleanField(obj, gFieldIds->mWOFFEnabled);
         s->setWOFFEnabled(flag);
//SAMSUNG CHANGES WOFF >>
        // This is required to enable the XMLTreeViewer when loading an XML document that
        // has no style attached to it. http://trac.webkit.org/changeset/79799
        s->setDeveloperExtrasEnabled(true);
        s->setSpatialNavigationEnabled(true);
        bool echoPassword = env->GetBooleanField(obj,
                gFieldIds->mPasswordEchoEnabled);
        s->setPasswordEchoEnabled(echoPassword);
// 4.2 Merge BEGIN <<
        flag = env->GetBooleanField(obj, gFieldIds->mMediaPlaybackRequiresUserGesture);
        s->setMediaPlaybackRequiresUserGesture(flag);
// 4.2 Merge END >>

//SAMSUNG CHANGES IMIDEO SERVICE >>
        jint mode = env->GetIntField(obj, gFieldIds->mEnableImideo);
        s->setImideoDebugMode(mode);
//SAMSUNG CHANGES IMIDEO SERVICE <<
    }
};


//-------------------------------------------------------------
// JNI registration
//-------------------------------------------------------------

static JNINativeMethod gWebSettingsMethods[] = {
    { "nativeSync", "(I)V",
        (void*) WebSettings::Sync }
};

int registerWebSettings(JNIEnv* env)
{
    jclass clazz = env->FindClass("android/webkit/WebSettingsClassic");
    ALOG_ASSERT(clazz, "Unable to find class WebSettingsClassic!");
    gFieldIds = new FieldIds(env, clazz);
    env->DeleteLocalRef(clazz);
    return jniRegisterNativeMethods(env, "android/webkit/WebSettingsClassic",
            gWebSettingsMethods, NELEM(gWebSettingsMethods));
}

}
