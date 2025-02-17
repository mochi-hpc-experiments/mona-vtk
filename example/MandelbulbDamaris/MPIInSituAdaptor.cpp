#include "MPIInSituAdaptor.hpp"

#include <mpi.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkCommunicator.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkMPI.h>
#include <vtkMPICommunicator.h>
#include <vtkMPIController.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>

#include "mb.hpp"
#include <iostream>

#ifdef DEBUG_BUILD
#define DEBUG(x) std::cout << x << std::endl;
#else
#define DEBUG(x)                                                                                   \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

namespace
{
vtkMultiProcessController* Controller = nullptr;
vtkCPProcessor* Processor = nullptr;
vtkMultiBlockDataSet* VTKGrid;

// one process generates one data object
void BuildVTKGrid(Mandelbulb& grid, int nprocs, int rank)
{
  int* extents = grid.GetExtents();
  vtkNew<vtkImageData> imageData;
  imageData->SetSpacing(1.0 / nprocs, 1, 1);
  imageData->SetExtent(extents);
  imageData->SetOrigin(
    grid.GetOrigin()); // Not necessary for (0,0,0) // the origin is different for different block
  vtkNew<vtkMultiPieceDataSet> multiPiece;
  multiPiece->SetNumberOfPieces(nprocs);
  multiPiece->SetPiece(rank, imageData.GetPointer());
  VTKGrid->SetNumberOfBlocks(1);
  VTKGrid->SetBlock(0, multiPiece.GetPointer());
}

// one process generates multiple data objects
void BuildVTKGridList(std::vector<Mandelbulb>& gridList, int global_blocks)
{
  int local_piece_num = gridList.size();
  vtkNew<vtkMultiPieceDataSet> multiPiece;
  multiPiece->SetNumberOfPieces(local_piece_num);

  for (int i = 0; i < local_piece_num; i++)
  {
    int* extents = gridList[i].GetExtents();
    vtkNew<vtkImageData> imageData;
    imageData->SetSpacing(1.0 / global_blocks, 1, 1);
    imageData->SetExtent(extents);
    imageData->SetOrigin(gridList[i].GetOrigin());
    multiPiece->SetPiece(i, imageData.GetPointer());
  }

  // one block conains one multipiece, one multipiece contains multiple actual data objects
  if (local_piece_num == 0)
  {
    // when there is no grid, and it is dummy node
    VTKGrid->SetNumberOfBlocks(0);
  }
  else
  {
    VTKGrid->SetNumberOfBlocks(1);
    VTKGrid->SetBlock(0, multiPiece.GetPointer());
  }
}

void UpdateVTKAttributes(Mandelbulb& mandelbulb, int rank, vtkCPInputDataDescription* idd)
{
  vtkMultiPieceDataSet* multiPiece = vtkMultiPieceDataSet::SafeDownCast(VTKGrid->GetBlock(0));
  if (idd->IsFieldNeeded("mandelbulb", vtkDataObject::POINT))
  {
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(multiPiece->GetPiece(rank));
    if (dataSet->GetPointData()->GetNumberOfArrays() == 0)
    {
      // pressure array
      vtkNew<vtkIntArray> data;
      data->SetName("mandelbulb");
      data->SetNumberOfComponents(1);
      dataSet->GetPointData()->AddArray(data.GetPointer());
    }
    vtkIntArray* data = vtkIntArray::SafeDownCast(dataSet->GetPointData()->GetArray("mandelbulb"));
    // The pressure array is a scalar array so we can reuse
    // memory as long as we ordered the points properly.
    int* theData = mandelbulb.GetData();
    data->SetArray(theData, static_cast<vtkIdType>(mandelbulb.GetNumberOfLocalCells()), 1);
  }
}

void UpdateVTKAttributesList(
  std::vector<Mandelbulb>& mandelbulbList, vtkCPInputDataDescription* idd)
{
  int pieceNum = mandelbulbList.size();
  if (pieceNum > 0)
  {
    // there is piece only when there is datablock
    vtkMultiPieceDataSet* multiPiece = vtkMultiPieceDataSet::SafeDownCast(VTKGrid->GetBlock(0));
    if (idd->IsFieldNeeded("mandelbulb", vtkDataObject::POINT))
    {
      for (int i = 0; i < pieceNum; i++)
      {
        vtkDataSet* dataSet = vtkDataSet::SafeDownCast(multiPiece->GetPiece(i));
        if (dataSet->GetPointData()->GetNumberOfArrays() == 0)
        {
          // pressure array
          vtkNew<vtkIntArray> data;
          data->SetName("mandelbulb");
          data->SetNumberOfComponents(1);
          dataSet->GetPointData()->AddArray(data.GetPointer());
        }
        vtkIntArray* data =
          vtkIntArray::SafeDownCast(dataSet->GetPointData()->GetArray("mandelbulb"));
        // The pressure array is a scalar array so we can reuse
        // memory as long as we ordered the points properly.
        // std::cout << "set actual value for piece " << i << std::endl;
        int* theData = mandelbulbList[i].GetData();
        data->SetArray(
          theData, static_cast<vtkIdType>(mandelbulbList[i].GetNumberOfLocalCells()), 1);
      }
    }
  }
}

void BuildVTKDataStructures(
  Mandelbulb& mandelbulb, int nprocs, int rank, vtkCPInputDataDescription* idd)
{
  if (VTKGrid == NULL)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    VTKGrid = vtkMultiBlockDataSet::New();
    BuildVTKGrid(mandelbulb, nprocs, rank);
  }
  UpdateVTKAttributes(mandelbulb, rank, idd);
}

void BuildVTKDataStructuresList(
  std::vector<Mandelbulb>& mandelbulbList, int global_nblocks, vtkCPInputDataDescription* idd)
{

  // there is known issue if we delete VTKGrid every time
  if (VTKGrid == NULL)
  {
    VTKGrid = vtkMultiBlockDataSet::New();
  }

  // fill in actual values
  BuildVTKGridList(mandelbulbList, global_nblocks);
  UpdateVTKAttributesList(mandelbulbList, idd);
}
} // namespace

namespace InSitu
{

void Initialize(const std::string& script, MPI_Comm comm)
{
  DEBUG("InSituAdaptor MPIInitialize Start ");

  vtkMPICommunicatorOpaqueComm opaqueComm(&comm);
  if (Processor == NULL)
  {
    Processor = vtkCPProcessor::New();
    Processor->Initialize(opaqueComm);
  }
  else
  {
    Processor->RemoveAllPipelines();
  }

  vtkNew<vtkCPPythonScriptPipeline> pipeline;
  pipeline->Initialize(script.c_str());

  Processor->AddPipeline(pipeline.GetPointer());
  DEBUG("InSituAdaptor MPIInitialize Finish ");
}

void Finalize()
{
  if (Processor)
  {
    Processor->Delete();
    Processor = NULL;
  }
  if (VTKGrid)
  {
    VTKGrid->Delete();
    VTKGrid = NULL;
  }
}

void CoProcess(std::vector<Mandelbulb>& mandelbulbList,
  int global_nblocks, double time, unsigned int timeStep)
{
  // actual execution of the coprocess
  // this is for vti
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("input");
  dataDescription->SetTimeData(time, timeStep);

  if (Processor->RequestDataDescription(dataDescription.GetPointer()) != 0)
  {
    // TODO use the blocknumber and blockid
    // std::cout << "debug list size " << mandelbulbList.size() << " for rank " << rank <<
    // std::endl;
    vtkCPInputDataDescription* idd = dataDescription->GetInputDescriptionByName("input");
    BuildVTKDataStructuresList(mandelbulbList, global_nblocks, idd);
    idd->SetGrid(VTKGrid);
    Processor->CoProcess(dataDescription.GetPointer());
  }
}

} // namespace InSitu
