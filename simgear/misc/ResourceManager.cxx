// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

/**
 * @file
 * @brief  manage finding resources by names/paths
 */

#include <algorithm>
#include <cassert>
#include <mutex>
#include <vector>

#include <simgear/debug/debug_types.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sg_path.hxx>

namespace simgear
{

static inline std::mutex static_manager_mutex;
static ResourceManager* static_manager = nullptr;

std::ostream& operator<<(std::ostream& stream, ResourceManager::FileType type)
{
    switch (type) {
    case ResourceManager::FileType::Font:
        stream << "Font";
        break;
    default:
        stream << "Unknown";
    }
    return stream;
}

ResourceProvider::~ResourceProvider()
{
    // pin to this compilation unit
}

void ResourceProvider::findAllOfTypeHelper(
    SGPath path, ResourceManager::FileType type,
    std::vector<SGPath>& result) const
{
    if (!path.exists()) {
        return;
    }
    switch (type) {
    case ResourceManager::FileType::Font:
        path.append("Fonts");
        break;
    default:
        SG_LOG(SG_IO, SG_DEV_WARN, "ResourceManager::findAllOfType: type '" << type << "' not handled");
        return;
    }
    findAllOfTypeHelper(Dir(path), type, result);
}

void ResourceProvider::findAllOfTypeHelper(
    const Dir& dir, ResourceManager::FileType type,
    std::vector<SGPath>& result) const
{
    if (!dir.exists()) {
        return;
    }
    for (const auto& c : dir.children()) {
        if (c.isDir()) {
            findAllOfTypeHelper(c, type, result);
        } else if (c.isFile()) {
            const std::string suffix = c.complete_lower_extension();
            if (type == ResourceManager::FileType::Font) {
                if (suffix == "ttf" || suffix == "otf") {
                    result.push_back(c);
                }
            }
        }
    }
}

ResourceManager::ResourceManager()
{
}

ResourceManager* ResourceManager::instance()
{
    const std::lock_guard<std::mutex> lock(static_manager_mutex);
    if (!static_manager) {
        static_manager = new ResourceManager();
    }

    return static_manager;
}

bool ResourceManager::haveInstance()
{
    const std::lock_guard<std::mutex> lock(static_manager_mutex);
    return static_manager != nullptr;
}

ResourceManager::~ResourceManager()
{
    const std::lock_guard<std::mutex> lock(static_manager_mutex);
    assert(this == static_manager);
    static_manager = nullptr;
    std::for_each(_providers.begin(), _providers.end(),
                  [](ResourceProvider* p) { delete p; });
}

void ResourceManager::reset()
{
    const std::lock_guard<std::mutex> lock(static_manager_mutex);
    if (static_manager) {
        delete static_manager;
        static_manager = nullptr;
    }
}

/**
 * trivial provider using a fixed base path
 */
class BasePathProvider : public ResourceProvider
{
public:
    BasePathProvider(const SGPath& aBase, ResourceManager::Priority aPriority) :
        ResourceProvider(aPriority),
        _base(aBase)
    {
    }

    virtual PathList findAllOfType(ResourceManager::FileType type) const override
    {
        std::vector<SGPath> paths;
        findAllOfTypeHelper(_base, type, paths);
        return paths;
    }

    virtual SGPath resolve(const std::string& aResource, SGPath&) const override
    {
        SGPath p(_base, aResource);
        return p.exists() ? p : SGPath();
    }
private:
    SGPath _base;
};

void ResourceManager::addBasePath(const SGPath& aPath, Priority aPriority)
{
    addProvider(new BasePathProvider(aPath, aPriority));
}

void ResourceManager::addProvider(ResourceProvider* aProvider)
{
    assert(aProvider);

    ProviderVec::iterator it = _providers.begin();
    for (; it != _providers.end(); ++it) {
      if (aProvider->priority() > (*it)->priority()) {
        _providers.insert(it, aProvider);
        return;
      }
    }

    // fell out of the iteration, goes to the end of the vec
    _providers.push_back(aProvider);
}

void ResourceManager::removeProvider(ResourceProvider* aProvider)
{
    assert(aProvider);
    auto it = std::find(_providers.begin(), _providers.end(), aProvider);
    if (it == _providers.end()) {
        SG_LOG(SG_GENERAL, SG_DEV_ALERT, "Cannot remove unregistered resource provider");
        return;
    }

    _providers.erase(it);
}

SGPath ResourceManager::findPath(const std::string& aResource, SGPath aContext)
{
    const SGPath completePath(aContext, aResource);

    if (!aContext.isNull() && completePath.exists()) {
        return completePath;
    }

    // Absolute, existing path and SGPath::validate() grants read access -> OK
    if (completePath.isAbsolute()) {
        const auto authorizedPath = completePath.validate(false);
        if (!authorizedPath.isNull() && authorizedPath.exists()) {
            return authorizedPath;
        }
    }

    // Loop over the resource providers even if 'completePath' is absolute.
    // For instance, when readProperties() processes 'include' attributes, it
    // is expected that 'aResource' be interpreted relatively to 'aContext'
    // or, if this doesn't lead to an existing file, a data path like
    // $FG_ROOT. In the latter case, BasePathProvider will do the job.
    for (const auto& provider : _providers) {
        const SGPath path = provider->resolve(aResource, aContext);
        if (!path.isNull()) {
            return path;
        }
    }

    return SGPath();
}

std::vector<SGPath> ResourceManager::findAllOfType(FileType type)
{
    std::vector<SGPath> paths;
    for (const auto& provider : _providers) {
        std::vector<SGPath> foundPaths = provider->findAllOfType(type);
        paths.insert(paths.end(), foundPaths.begin(), foundPaths.end());
    }
    return paths;
}

} // of namespace simgear
