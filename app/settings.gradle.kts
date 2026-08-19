pluginManagement {
    includeBuild("../miuix/build-plugins")
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        maven("https://jitpack.io")
    }
}

plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "1.0.0"
}

includeBuild("../miuix")
includeBuild("../AndroidLiquidGlass") {
    dependencySubstitution {
        substitute(module("io.github.kyant0:backdrop")).using(project(":backdrop"))
    }
}

rootProject.name = "QcomBandMenu"
include(":app")
