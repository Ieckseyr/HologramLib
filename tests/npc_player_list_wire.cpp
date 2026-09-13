#include "playernpc/NpcPlayerList.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace debugshape_export::npc_protocol;
using Action = sculk::protocol::PlayerListPacket::ActionType;

static void require(bool condition, char const* message) {
    if (!condition) throw std::runtime_error(message);
}

static void save(std::filesystem::path const& path, std::vector<std::byte> const& bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<char const*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(static_cast<bool>(file), "could not write wire sample");
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: npc_player_list_wire <output-directory>");
        std::filesystem::path const output(argv[1]);
        std::filesystem::create_directories(output);

        // PNG path: existing Id, absent FullId. Also models a pre-fix persisted blob.
        sculk::protocol::SerializedSkin skin;
        skin.mId = "HoloLibNpcSkin_wire-check";
        finalizeSkinIds(skin, "wire-check");
        require(skin.mFullId == "HoloLibNpcSkin_wire-check", "PNG/blob FullId was not populated");

        // Capture path: Persona rewrites Id after an old FullId was copied.
        sculk::protocol::SerializedSkin captured;
        captured.mId = "hl_npc_42";
        captured.mFullId = "persona-source-player";
        finalizeSkinIds(captured, "captured");
        require(captured.mFullId == "hl_npc_42", "captured FullId still refers to the original appearance");

        // Recovery from a blob with no internal Id still has the registry key.
        sculk::protocol::SerializedSkin restored;
        finalizeSkinIds(restored, "restored");
        require(restored.mId == "HoloLibNpcSkin_restored" && restored.mFullId == restored.mId,
                "restored identity is empty or inconsistent");

        skin.mResourcePatch = R"({"geometry":{"default":"geometry.humanoid.custom"}})";
        skin.mSkinImageWidth = 64;
        skin.mSkinImageHeight = 64;
        skin.mSkinImageBytes.assign(64 * 64 * 4, '\xff');
        skin.mGeometryData = "{}"; // Wire fixture only; not a client render fixture.
        skin.mGeometryDataMinEngineVersion = "1.12.0";
        skin.mArmSize = "wide";
        skin.mSkinColor = "#ff123456";
        skin.mOverridesPlayerAppearance = true;
        // An old false value must not defeat the trusted NPC PlayerList entry.
        skin.mTrustedSkinFlag = "false";

        sculk::protocol::PlayerListPacket add;
        add.mAction = Action::Add;
        add.mPlayerEntryList = {playerListEntry(npcUuid(42), 0x6F000001, "NPC-wire-check", skin)};
        std::vector<std::byte> addBody;
        sculk::protocol::BinaryStream addStream(addBody);
        add.write(addStream); // Exactly the external serializer used by sendToPlayer.
        require(hasValidPlayerListPrefix(add, addBody), "linked Protocol.lib emitted a pre-2168 Add frame");
        save(output / "PlayerList-add.body.bin", addBody);

        sculk::protocol::PlayerListPacket remove;
        remove.mAction = Action::Remove;
        remove.mPlayerEntryList = {playerListEntry(npcUuid(42), 0, "", {})};
        std::vector<std::byte> removeBody;
        sculk::protocol::BinaryStream removeStream(removeBody);
        remove.write(removeStream);
        require(hasValidPlayerListPrefix(remove, removeBody), "linked Protocol.lib emitted a pre-2168 Remove frame");
        save(output / "PlayerList-remove.body.bin", removeBody);

        std::vector<std::byte> legacyAdd{std::byte{0}, std::byte{1}};
        legacyAdd.insert(legacyAdd.end(), addBody.begin() + 3, addBody.end());
        require(!hasValidPlayerListPrefix(add, legacyAdd), "legacy Add was accepted as 2168");
        std::vector<std::byte> legacyRemove{std::byte{1}, std::byte{1}};
        legacyRemove.insert(legacyRemove.end(), removeBody.begin() + 3, removeBody.end());
        require(!hasValidPlayerListPrefix(remove, legacyRemove), "legacy Remove was accepted as 2168");
        require(!hasValidPlayerListPrefix(add, std::span(addBody).first(2)), "truncated header was accepted");

        // NetworkPeer receives a varuint packet header followed by these exact body bytes.
        std::vector<std::byte> framed;
        sculk::protocol::BinaryStream frameStream(framed);
        frameStream.writeUnsignedVarInt(static_cast<std::uint32_t>(add.getId()) & 0x3ff);
        frameStream.writeBytes(addBody.data(), addBody.size());
        save(output / "PlayerList-add.packet.bin", framed);

        std::cout << "protocol=" << SCULK_NETWORK_PROTOCOL_VERSION
                  << " addBytes=" << addBody.size() << " addPrefix=[" << bodyPrefixHex(addBody)
                  << "] removeBytes=" << removeBody.size() << " removePrefix=[" << bodyPrefixHex(removeBody)
                  << "]\nSkin identity and framing regressions passed.\n";
        return 0;
    } catch (std::exception const& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
