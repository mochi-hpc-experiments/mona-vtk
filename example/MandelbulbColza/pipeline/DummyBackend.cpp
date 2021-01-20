/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "DummyBackend.hpp"
#include "../InSituAdaptor.hpp"
#include <iostream>

COLZA_REGISTER_BACKEND(dummy, DummyPipeline);

// this function is called by colza framework when the pipeline is created
void DummyPipeline::updateMonaAddresses(
  mona_instance_t mona, const std::vector<na_addr_t>& addresses)
{
  std::cout << "updateMonaAddresses is called" << std::endl;

  // try to init the monaCommunicator it is called the first time
  if (firstUpdateMona)
  {
    // create the mona communicator
    // there are seg fault here if we create themm multiple times without the condition of firstUpdateMona
    na_return_t ret =
      mona_comm_create(mona, addresses.size(), addresses.data(), &(this->m_mona_comm));
    if (ret != 0)
    {
      throw std::runtime_error("failed to init mona");
    }

    int procSize;
    int procRank;
    mona_comm_size(this->m_mona_comm, &procSize);
    mona_comm_rank(this->m_mona_comm, &procRank);
    std::cout << "Init mona, mona addresses have been updated, size is " << procSize << " rank is " << procRank
              << std::endl;

    // init the mochi communicator
    // this is supposed to be called once
    // TODO set this from the client or server parameters?
    std::string scriptname =
      "/global/homes/z/zw241/cworkspace/src/mona-vtk/example/MandelbulbColza/pipeline/render.py";
    // also init the insitu adaptor
    InSitu::MonaInitialize(scriptname, this->m_mona_comm);
    firstUpdateMona = false;
  }
}

colza::RequestResult<int32_t> DummyPipeline::start(uint64_t iteration)
{
  std::cerr << "Iteration " << iteration << " starting" << std::endl;
  colza::RequestResult<int32_t> result;
  result.success() = true;
  result.value() = 0;
  return result;
}

void DummyPipeline::abort(uint64_t iteration)
{
  std::cerr << "Client aborted iteration " << iteration << std::endl;
}

// update the data, try to add the visulization operations
colza::RequestResult<int32_t> DummyPipeline::execute(uint64_t iteration)
{
  (void)iteration;
  auto result = colza::RequestResult<int32_t>();
  result.value() = 0;
  return result;
}

colza::RequestResult<int32_t> DummyPipeline::cleanup(uint64_t iteration)
{
  std::lock_guard<tl::mutex> g(m_datasets_mtx);
  m_datasets.erase(iteration);
  auto result = colza::RequestResult<int32_t>();
  result.value() = 0;
  return result;
}

colza::RequestResult<int32_t> DummyPipeline::stage(const std::string& sender_addr,
  const std::string& dataset_name, uint64_t iteration, uint64_t block_id,
  const std::vector<size_t>& dimensions, const std::vector<int64_t>& offsets,
  const colza::Type& type, const thallium::bulk& data)
{
  colza::RequestResult<int32_t> result;
  result.value() = 0;
  {
    std::lock_guard<tl::mutex> g(m_datasets_mtx);
    if (m_datasets.count(iteration) != 0 && m_datasets[iteration].count(dataset_name) != 0 &&
      m_datasets[iteration][dataset_name].count(block_id) != 0)
    {
      result.error() = "Block already exists for provided iteration, name, and id";
      result.success() = false;
      return result;
    }
  }
  DataBlock block;
  block.dimensions = dimensions;
  block.offsets = offsets;
  block.type = type;
  block.data.resize(data.size());

  try
  {
    std::vector<std::pair<void*, size_t> > segments = { std::make_pair<void*, size_t>(
      block.data.data(), block.data.size()) };
    auto local_bulk = m_engine.expose(segments, tl::bulk_mode::write_only);
    auto origin_ep = m_engine.lookup(sender_addr);
    data.on(origin_ep) >> local_bulk;
  }
  catch (const std::exception& ex)
  {
    result.success() = false;
    result.error() = ex.what();
  }

  if (result.success())
  {
    std::lock_guard<tl::mutex> g(m_datasets_mtx);
    m_datasets[iteration][dataset_name][block_id] = std::move(block);
  }
  return result;
}

colza::RequestResult<int32_t> DummyPipeline::destroy()
{
  colza::RequestResult<int32_t> result;
  result.value() = true;
  return result;
}

std::unique_ptr<colza::Backend> DummyPipeline::create(const colza::PipelineFactoryArgs& args)
{
  return std::unique_ptr<colza::Backend>(new DummyPipeline(args));
}
