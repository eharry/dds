/**
 *    Copyright (C) 2010-2015 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kSharding

#include "mongo/platform/basic.h"

#include "mongo/s/grid.h"

#include "mongo/base/status_with.h"
#include "mongo/client/connpool.h"
#include "mongo/s/catalog/catalog_cache.h"
#include "mongo/s/catalog/catalog_manager.h"
#include "mongo/s/catalog/type_settings.h"
#include "mongo/s/catalog/type_shard.h"
#include "mongo/s/client/shard_registry.h"
#include "mongo/s/config.h"
#include "mongo/util/fail_point_service.h"
#include "mongo/util/log.h"

namespace mongo {

    using boost::shared_ptr;
    using std::endl;
    using std::map;
    using std::set;
    using std::string;
    using std::vector;

    Grid::Grid() : _allowLocalShard(true) {

    }

    void Grid::setCatalogManager(std::unique_ptr<CatalogManager> catalogManager) {
        invariant(!_catalogManager);
        invariant(!_catalogCache);
        invariant(!_shardRegistry);

        _catalogManager = std::move(catalogManager);
        _catalogCache = stdx::make_unique<CatalogCache>(_catalogManager.get());
        _shardRegistry = stdx::make_unique<ShardRegistry>(_catalogManager.get());
    }

    StatusWith<shared_ptr<DBConfig>> Grid::implicitCreateDb(const std::string& dbName) {
        auto status = catalogCache()->getDatabase(dbName);
        if (status.isOK()) {
            return status;
        }

        if (status == ErrorCodes::DatabaseNotFound) {
            auto statusCreateDb = catalogManager()->createDatabase(dbName, NULL);
            if (statusCreateDb.isOK() || statusCreateDb == ErrorCodes::NamespaceExists) {
                return catalogCache()->getDatabase(dbName);
            }

            return statusCreateDb;
        }

        return status;
    }

    bool Grid::allowLocalHost() const {
        return _allowLocalShard;
    }

    void Grid::setAllowLocalHost( bool allow ) {
        _allowLocalShard = allow;
    }

    /*
     * Returns whether balancing is enabled, with optional namespace "ns" parameter for balancing on a particular
     * collection.
     */
    bool Grid::shouldBalance(const SettingsType& balancerSettings) const {
        if (balancerSettings.isBalancerStoppedSet() && balancerSettings.getBalancerStopped()) {
            return false;
        }

        if (balancerSettings.isBalancerActiveWindowSet()) {
            boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
            return balancerSettings.inBalancingWindow(now);
        }

        return true;
    }

    bool Grid::getConfigShouldBalance() const {
        auto balSettingsResult =
            grid.catalogManager()->getGlobalSettings(SettingsType::BalancerDocKey);
        if (!balSettingsResult.isOK()) {
            warning() << balSettingsResult.getStatus();
            return false;
        }
        SettingsType balSettings = balSettingsResult.getValue();

        if (!balSettings.isKeySet()) {
            // Balancer settings doc does not exist. Default to yes.
            return true;
        }

        return shouldBalance(balSettings);
    }

    Grid grid;
}
