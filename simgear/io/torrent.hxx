/*
SPDX-License-Identifier: LGPL-2.0-or-later
SPDX-FileCopyrightText: 2025 Julian Smith
*/

#pragma once

#ifdef SG_TORRENT

#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "libtorrent/torrent_info.hpp"

#include <functional>
#include <string>


namespace simgear
{

/* Support for using Torrents. */
struct Torrent : SGSubsystem
{
    /* <node> is populated like this:
        node/
            stats_metrics/
                All available libtorrent::stats_metric's.
                See:
                    https://libtorrent.org/reference-Stats.html
                    https://libtorrent.org/manual-ref.html#session-statistics
            
            session_error           (libtorrent::session_error_alert)
            incoming_connections    (libtorrent::incoming_connection_alert)
            incoming_requests       (libtorrent::incoming_request_alert)
            
            torrent[]/  One item per torrent.
            
                error_filename  (torrent_error_alert)
                error_message   (torrent_error_alert)
                path    (outpath of torrent)
                paused  (libtorrent::torrent_paused_alert, libtorrent::torrent_resumed_alert)
                status
                    init
                    checked     (libtorrent::torrent_checked_alert)
                    finished    (libtorrent::torrent_finished_alert)
                    error       (libtorrent::torrent_error_alert)
                torrent (Path of .torrent file.)
                
                status/
                    (All items in libtorrent::torrent_status`: https://libtorrent.org/reference-Torrent_Status.html.)
                peers/ip_.../
                    (All items in `libtorrent::peer_info`: https://libtorrent.org/reference-Core.html.)
                    client
                    flags
                    ...
    */
    Torrent(
            SGPropertyNode_ptr node,
            std::function<simgear::HTTP::Client* ()> get_http_client
            );
    
    // Subsystem API.
    void init() override;
    void shutdown() override;
    void unbind() override;
    void update(double delta_time_sec) override;
    
    // Subsystem identification.
    static const char* staticSubsystemClassId();
    
    // Torrent operations.
    
    /* Called with true/false on success/failure of torrent operation. */
    using fn_result_callback = std::function<void (bool ok)>;
    
    /* Called after torrent file has been loaded, before download. */
    using fn_info_callback = std::function<void (std::shared_ptr<libtorrent::torrent_info> torrent_info)>;
    
    /* Reads specified torrent file, calls info_callback(), downloads to
    <out_path>, then calls result_callback(). */
    void add_torrent(
            SGPath& torrent_path,
            SGPath& out_path,
            fn_result_callback result_callback,
            fn_info_callback info_callback = nullptr
            );
    
    /* Downloads .torrent file from <torrent_url> to <torrent_path> then calls
    add_torrent(). */
    void add_torrent_url(
            const std::string& torrent_url,
            SGPath& torrent_path,
            SGPath& out_path,
            fn_result_callback result_callback,
            fn_info_callback info_callback = nullptr
            );
};

}   // namespace simgear


#endif
