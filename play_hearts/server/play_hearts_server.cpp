/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "play_hearts/server/PlayerSession.h"

#include "play_hearts.grpc.pb.h"
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using playhearts::PlayHearts;

class PlayHeartsImpl final : public PlayHearts::Service
{
public:
  explicit PlayHeartsImpl(const char* modelpath)
      : mModelPath(modelpath)
  {
    assert(mModelPath != nullptr);
  }

  Status Connect(ServerContext*, ServerReaderWriter<ServerMessage, ClientMessage>* stream) override
  {
    assert(mModelPath != nullptr);
    PlayerSession session(stream, mModelPath);
    return session.ManageSession();
  }

private:
  const char* mModelPath;
};

void RunServer(const char* modelpath)
{
  assert(modelpath != nullptr);

  std::string server_address("0.0.0.0:50057");
  PlayHeartsImpl service(modelpath);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv)
{
  assert(argc == 2);

  const char* modelpath = argv[1];
  assert(modelpath != nullptr);
  RunServer(modelpath);

  return 0;
}
