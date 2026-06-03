import com.android.build.gradle.internal.api.BaseVariantOutputImpl
import java.util.Properties

plugins {
    id("com.android.library")
}

val versionProps = Properties().apply {
    file("${rootProject.projectDir}/gradle.properties").inputStream().use { load(it) }
}
val endeeVersion: String = versionProps.getProperty("endeeVersion", "0.1.0")

android {
    namespace = "io.endee.ndd"
    compileSdk = 34

    defaultConfig {
        minSdk = 24

        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DNDD_BUILD_ANDROID_JNI=ON",
                    "-DUSE_NEON=ON",
                    "-DCMAKE_SHARED_LINKER_FLAGS=-Wl,-z,max-page-size=16384",
                )
                cppFlags += listOf("-std=c++20")
            }
        }

        consumerProguardFiles("consumer-rules.pro")
    }

    buildTypes {
        debug {
            isMinifyEnabled = false
        }
        release {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    sourceSets {
        getByName("main") {
            java.srcDirs("../java")
            manifest.srcFile("src/main/AndroidManifest.xml")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.22.1"
        }
    }

    ndkVersion = "27.2.12479018"

    packaging {
        jniLibs {
            useLegacyPackaging = false
        }
    }
}

@Suppress("DEPRECATION")
android.libraryVariants.configureEach {
    outputs.configureEach {
        (this as BaseVariantOutputImpl).outputFileName =
            "endee-vdb-${buildType.name}-${endeeVersion}.aar"
    }
}

val releaseDir = layout.projectDirectory.dir("../release")

fun registerStageAarTask(variantName: String, buildType: String) {
    val capitalized = variantName.replaceFirstChar { it.uppercase() }
    tasks.register<Copy>("stage${capitalized}Aar") {
        dependsOn("bundle${capitalized}Aar")
        from(layout.buildDirectory.dir("outputs/aar")) {
            include("endee-vdb-${buildType}-${endeeVersion}.aar")
        }
        into(releaseDir)
    }
}

registerStageAarTask("release", "release")
registerStageAarTask("debug", "debug")
