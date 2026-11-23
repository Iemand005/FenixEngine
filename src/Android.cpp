#include "Android.hpp"
#include "Log.hpp"

#include <string>

#if defined(__ANDROID__)
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_filesystem.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "WawaDir.hpp"

namespace fe {

	std::string EngineResourceBaseDir()
	{
#if defined(__ANDROID__)
		const char* internalPath = SDL_GetAndroidInternalStoragePath();
		if (internalPath) return std::string(internalPath);
		return std::string();
#else
		return GetExecutableDirectory();
#endif
	}

#if defined(__ANDROID__)

	namespace {
		// Copies a single file from the APK into internal storage (destRoot/relPath).
		void AndroidCopyFile(AAssetManager* assets, const std::string& destRoot, const std::string& relPath)
		{
			AAsset* asset = AAssetManager_open(assets, relPath.c_str(), AASSET_MODE_STREAMING);
			if (!asset) return;

			std::string outPath = destRoot + "/" + relPath;
			size_t lastSlash = outPath.find_last_of('/');
			if (lastSlash != std::string::npos) {
				std::string dir = outPath.substr(0, lastSlash);
				if (!SDL_CreateDirectory(dir.c_str())) {
					SDL_ClearError();
				}
			}
			int fd = open(outPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (fd >= 0) {
				char buf[16384];
				int n;
				while ((n = AAsset_read(asset, buf, sizeof(buf))) > 0) {
					ssize_t written = write(fd, buf, n);
					(void)written;
				}
				close(fd);
			}
			AAsset_close(asset);
		}

		// Recursively lists a directory using Java's AssetManager.list(), which
		// (unlike AAssetDir) reliably returns both files and subdirectories on all
		// Android versions. AAssetManager is only used for the actual file reads.
		void AndroidExtractDir(JNIEnv* env, jobject assetManager, jmethodID listMethod,
			AAssetManager* assets, const std::string& destRoot, const std::string& relDir)
		{
			jstring jrel = env->NewStringUTF(relDir.c_str());
			jobjectArray arr = (jobjectArray)env->CallObjectMethod(assetManager, listMethod, jrel);
			env->DeleteLocalRef(jrel);
			if (!arr) {
				LogWarning("AndroidSetupAssets: list failed for '%s'", relDir.empty() ? "(root)" : relDir.c_str());
				return;
			}
			jsize n = env->GetArrayLength(arr);
			int files = 0;
			for (jsize i = 0; i < n; ++i) {
				jstring jname = (jstring)env->GetObjectArrayElement(arr, i);
				const char* name = env->GetStringUTFChars(jname, NULL);
				if (name) {
					std::string rel = relDir.empty() ? std::string(name) : relDir + "/" + std::string(name);

					// Open as a file; if we get NULL it's a directory, so recurse into it.
					AAsset* probe = AAssetManager_open(assets, rel.c_str(), AASSET_MODE_UNKNOWN);
					if (probe) {
						AAsset_close(probe);
						AndroidCopyFile(assets, destRoot, rel);
						++files;
					} else {
						AndroidExtractDir(env, assetManager, listMethod, assets, destRoot, rel);
					}
					env->ReleaseStringUTFChars(jname, name);
				}
				env->DeleteLocalRef(jname);
			}
			env->DeleteLocalRef(arr);
			Log("AndroidSetupAssets: '%s' -> %d entries, %d files", relDir.empty() ? "(root)" : relDir.c_str(), (int)n, files);
		}
	} // namespace

	bool AndroidSetupAssets(const std::string& assetDir)
	{
		static bool done = false;
		if (done) return true;
		done = true;

		JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
		if (!env) { LogError("AndroidSetupAssets: no JNI env"); return false; }
		jobject activity = (jobject)SDL_GetAndroidActivity();
		if (!activity) { LogError("AndroidSetupAssets: no activity"); return false; }
		jclass activityClass = env->GetObjectClass(activity);
		jmethodID getAssets = env->GetMethodID(activityClass, "getAssets", "()Landroid/content/res/AssetManager;");
		env->DeleteLocalRef(activityClass);
		if (!getAssets) { LogError("AndroidSetupAssets: no getAssets method"); return false; }
		jobject assetManager = env->CallObjectMethod(activity, getAssets);
		env->DeleteLocalRef(activity);
		if (!assetManager) { LogError("AndroidSetupAssets: no assetManager"); return false; }

		jclass assetManagerClass = env->GetObjectClass(assetManager);
		jmethodID list = env->GetMethodID(assetManagerClass, "list", "(Ljava/lang/String;)[Ljava/lang/String;");
		env->DeleteLocalRef(assetManagerClass);
		if (!list) { LogError("AndroidSetupAssets: no list method"); return false; }

		AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
		if (!mgr) { LogError("AndroidSetupAssets: no native manager"); return false; }

		const char* internalPath = SDL_GetAndroidInternalStoragePath();
		if (!internalPath) { LogError("AndroidSetupAssets: no internal path"); return false; }
		std::string destRoot(internalPath);
		Log("AndroidSetupAssets: internal path = %s", internalPath);

		AndroidExtractDir(env, assetManager, list, mgr, destRoot, assetDir);

		env->DeleteLocalRef(assetManager);

		if (chdir(destRoot.c_str()) != 0) {
			LogError("AndroidSetupAssets: chdir failed (%s)", strerror(errno));
			return false;
		}
		Log("AndroidSetupAssets: chdir to %s OK", destRoot.c_str());
		return true;
	}

#else // !__ANDROID__

	bool AndroidSetupAssets(const std::string& assetDir)
	{
		(void)assetDir;
		return true;
	}

#endif // __ANDROID__

} // namespace fe