/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include <fstream>

#include "tao/json/msgpack/consume_string.hpp"
#include "tao/json/msgpack/from_binary.hpp"
#include "tao/json/msgpack/from_input.hpp"
#include "tao/json/msgpack/from_string.hpp"
#include "tao/json/msgpack/parts_parser.hpp"
#include "tao/json/msgpack/to_stream.hpp"
#include "tao/json/msgpack/to_string.hpp"

#include "RIFF.h" // from libgig

#include "utils.h"
#include "patch_io.h"
#include "engine/engine.h"
#include "messaging/messaging.h"

#include "json/engine_traits.h"

namespace scxt::patch_io
{
void addSCManifest(const std::unique_ptr<RIFF::File> &f, const std::string &type)
{
    std::map<std::string, std::string> manifest;
    manifest["version"] = "1";
    manifest["type"] = type;
    auto mmsg = tao::json::to_string(json::scxt_value(manifest));
    auto c = f->AddSubChunk('scmf', mmsg.size());
    auto d = (uint8_t *)c->LoadChunkData();
    memcpy(d, mmsg.data(), mmsg.size());
}

std::unordered_map<std::string, std::string> readSCManifest(const std::unique_ptr<RIFF::File> &f)
{
    auto c1 = f->GetSubChunk('scmf');
    std::string s((char *)c1->LoadChunkData());

    tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>> consumer;
    tao::json::events::from_string(consumer, s);
    auto jv = std::move(consumer.value);
    std::unordered_map<std::string, std::string> manifest;
    jv.to(manifest);

    return manifest;
}

void addSCDataChunk(const std::unique_ptr<RIFF::File> &f, const std::string &msg)
{
    auto c = f->AddSubChunk('scdt', msg.size());
    auto d = (uint8_t *)c->LoadChunkData();
    memcpy(d, msg.data(), msg.size());
}

std::string readSCDataChunk(const std::unique_ptr<RIFF::File> &f)
{
    auto cp = f->GetSubChunk('scdt');
    return std::string((char *)cp->LoadChunkData(), cp->GetSize());
}

bool saveMulti(const fs::path &p, const scxt::engine::Engine &e)
{
    SCLOG("Made it to the patch code " << p.u8string());

    try
    {
        auto msg = tao::json::msgpack::to_string(json::scxt_value(e));

        auto f = std::make_unique<RIFF::File>('SCXT');
        f->SetByteOrder(RIFF::endian_little);
        addSCManifest(f, "multi");
        addSCDataChunk(f, msg);

        // TODO: If embeeding samples, add a list here with them
        f->Save(p.u8string());
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG(e.Message);
    }
    return true;
}

bool loadMulti(const fs::path &p, scxt::engine::Engine &engine)
{
    SCLOG("loadMulti " << p.u8string());

    std::string payload;
    try
    {
        auto f = std::make_unique<RIFF::File>(p.u8string());
        auto manifest = readSCManifest(f);
        payload = readSCDataChunk(f);
    }
    catch (const RIFF::Exception &e)
    {
        SCLOG("RIFF::Exception " << e.Message);
        return false;
    }

    auto &cont = engine.getMessageController();
    if (cont->isAudioRunning)
    {
        cont->stopAudioThreadThenRunOnSerial([payload, &nonconste = engine](auto &e) {
            try
            {
                nonconste.stopAllSounds();
                scxt::json::unstreamEngineState(nonconste, payload, true);
                auto &cont = *e.getMessageController();
                cont.restartAudioThreadFromSerial();
            }
            catch (std::exception &err)
            {
                SCLOG("Unable to load [" << err.what() << "]");
            }
        });
    }
    else
    {
        try
        {
            engine.stopAllSounds();
            scxt::json::unstreamEngineState(engine, payload, true);
        }
        catch (std::exception &err)
        {
            SCLOG("Unable to load [" << err.what() << "]");
        }
    }
    return true;
}
} // namespace scxt::patch_io