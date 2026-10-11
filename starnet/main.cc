#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cpm_drive.h"
#include "starnet.h"
#include "starnet_port.h"
#include "starnet_session.h"
#include "starnet_server.h"
#include "udp_port.h"

using namespace std;

namespace {

struct Slot
{
  bool assigned = false;
  bool seen = false;
  uint8_t sequence = 0;
  unique_ptr<StarnetServer> server;
  vector<vector<uint8_t>> replies;
};

void SendReplies(UdpPort & udp,
                 const sockaddr_in & peer,
                 uint8_t station,
                 uint8_t sequence,
                 const vector<vector<uint8_t>> & replies)
{
  udp.SetPeer(peer);
  uint8_t count = (uint8_t)replies.size();
  for (size_t i = 0; i < replies.size(); ++i) {
    StarnetShim shim;
    shim.station = station;
    shim.sequence = sequence;
    shim.replyIndex = (uint8_t)i;
    shim.replyCount = count;
    udp.SetShim(shim);
    udp.Write(replies[i].data(), (int)replies[i].size());
  }
}

void Reject(UdpPort & udp, const sockaddr_in & peer, uint8_t sequence)
{
  udp.SetPeer(peer);
  StarnetShim shim;
  shim.station = Starnet::kAssignStation;
  shim.sequence = sequence;
  shim.replyIndex = 0;
  shim.replyCount = 0;
  udp.SetShim(shim);
  udp.Write(nullptr, 0);
}

int Claim(vector<Slot> & slots, int requested)
{
  if (requested == Starnet::kAssignStation) {
    for (int i = 0; i < Starnet::kStationCount; ++i) {
      if (!slots[(size_t)i].assigned)
        return i;
    }
    return -1;
  }
  if (requested < 0 || requested >= Starnet::kStationCount)
    return -1;
  return requested;
}

} // namespace

int main(int argc, char ** argv)
{
  string image;
  vector<string> driveSpecs;
  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "--image" && i + 1 < argc)
      image = argv[++i];
    else if (arg == "--drive" && i + 1 < argc)
      driveSpecs.push_back(argv[++i]);
    else {
      cerr << "usage: starnet-server [--image FILE] [--drive SPEC]...\n"
           << "  SPEC is A=dir:PATH or A=image:FILE,dpb=NAME\n";
      return 1;
    }
  }

  CpmDriveSet drives;
  if (!driveSpecs.empty()) {
    string error;
    if (!CheckCpmDrives(driveSpecs, error) || !drives.Mount(driveSpecs, error, nullptr)) {
      cerr << "starnet: " << error << endl;
      return 1;
    }
  }

  UdpPort udp;
  if (!udp.Bind(Starnet::kPort)) {
    cerr << "starnet: could not bind 127.0.0.1:" << Starnet::kPort << endl;
    return 1;
  }
  cerr << "starnet: listening on 127.0.0.1:" << Starnet::kPort << endl;

  vector<Slot> slots((size_t)Starnet::kStationCount);
  for (Slot & slot : slots)
    slot.server.reset(new StarnetServer(image, &drives));

  for (;;) {
    uint8_t wire[512];
    int n = udp.Read(wire, (int)sizeof(wire));
    if (n < 0)
      continue;

    StarnetShim shim = udp.Shim();
    sockaddr_in peer = udp.Peer();
    int id = Claim(slots, shim.station);
    if (id < 0) {
      Reject(udp, peer, shim.sequence);
      continue;
    }

    Slot & slot = slots[(size_t)id];
    if (!slot.assigned) {
      slot.assigned = true;
      cerr << "starnet: station " << id << endl;
    }

    if (slot.seen && slot.sequence == shim.sequence) {
      SendReplies(udp, peer, (uint8_t)id, shim.sequence, slot.replies);
      continue;
    }

    PacketPort port(wire, n);
    if (!StarnetSession::Serve(*slot.server, port))
      continue;

    slot.replies = port.Replies();
    slot.sequence = shim.sequence;
    slot.seen = true;
    SendReplies(udp, peer, (uint8_t)id, shim.sequence, slot.replies);
  }
}
