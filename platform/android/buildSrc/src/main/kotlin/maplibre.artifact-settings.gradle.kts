extra["mapLibreArtifactGroupId"] = "com.oovy.maplibre"
extra["mapLibreArtifactId"] = "android-sdk"
extra["mapLibreArtifactTitle"] = "OOVY MapLibre Android"
extra["mapLibreArtifactDescription"] = "OOVY MapLibre Android native fork"
extra["mapLibreDeveloperName"] = "OOVY"
extra["mapLibreDeveloperId"] = "oovy"
extra["mapLibreArtifactUrl"] = "https://github.com/oovy-dev/maplibre-native"
extra["mapLibreArtifactScmUrl"] = "scm:git:https://github.com/oovy-dev/maplibre-native.git"
extra["mapLibreArtifactLicenseName"] = "BSD"
extra["mapLibreArtifactLicenseUrl"] = "https://opensource.org/licenses/BSD-2-Clause"

val versionFilePath = rootDir.resolve("VERSION")
val versionName = if (versionFilePath.exists()) {
    versionFilePath.readText().trim()
} else {
    throw GradleException("VERSION file not found at ${versionFilePath.absolutePath}")
}

extra["versionName"] = versionName
