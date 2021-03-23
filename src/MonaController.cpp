/*=========================================================================

Program:   Visualization Toolkit
Module:    MonaController.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "MonaController.hpp"

#include <vtkIntArray.h>
#include <vtkObjectFactory.h>
#include <vtkOutputWindow.h>
#include <vtkSmartPointer.h>

#include <cassert>

#include "Mona.hpp"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#ifdef DEBUG_BUILD
#  define DEBUG(x) std::cout << x << std::endl;
#else
#  define DEBUG(x) do {} while (0)
#endif

int MonaController::Initialized = 0;
char MonaController::ProcessorName[MPI_MAX_PROCESSOR_NAME] = "";
int MonaController::UseSsendForRMI = 0;

// Output window which prints out the process id
// with the error or warning messages
class MonaOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(MonaOutputWindow, vtkOutputWindow);

  void DisplayText(const char* t) override
  {
    if (this->Controller && MonaController::Initialized)
    {
      cout << "Process id: " << this->Controller->GetLocalProcessId() << " >> ";
    }
    cout << t;
  }

  MonaOutputWindow() { this->Controller = 0; }

  friend class MonaController;

protected:
  MonaController* Controller;
  MonaOutputWindow(const MonaOutputWindow&);
  void operator=(const MonaOutputWindow&);
};

void MonaController::CreateOutputWindow()
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  MonaOutputWindow* window = new MonaOutputWindow;
  window->InitializeObjectBase();
  window->Controller = this;
  this->OutputWindow = window;
  vtkOutputWindow::SetInstance(this->OutputWindow);
}

vtkStandardNewMacro(MonaController);

//----------------------------------------------------------------------------
/*
MonaController::MonaController()
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  // If MPI was already initialized obtain rank and size.
  if (MonaController::Initialized)
  {
    this->InitializeCommunicator(MonaCommunicator::GetWorldCommunicator());
    // Copy MonaController::WorldRMICommunicataor which is created when
    // MPI is initialized
    MonaCommunicator* comm = MonaCommunicator::New();
    comm->CopyFrom(MonaController::WorldRMICommunicator);
    this->RMICommunicator = comm;
  }

  this->OutputWindow = 0;
}
*/

MonaController::MonaController()
{
  DEBUG( "replaced, mochiContorller call function: " << __FUNCTION__ );
  // If MPI was already initialized obtain rank and size.
  if (MonaController::Initialized)
  {
    this->InitializeCommunicator(MonaCommunicator::GetWorldCommunicator());
    // Copy MonaController::WorldRMICommunicataor which is created when
    // MPI is initialized
    // MonaCommunicator* comm = MonaCommunicator::New();
    // copy the shared pointer point to the same ColzaComm
    // comm->CopyFrom(MonaController::WorldRMICommunicator);
    // TODO, consider the RMICommunicator when it is needed
    this->RMICommunicator = NULL;
  }

  this->OutputWindow = 0;
}

//----------------------------------------------------------------------------
MonaController::~MonaController()
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  this->SetCommunicator(0);
  if (this->RMICommunicator)
  {
    this->RMICommunicator->Delete();
  }
}

//----------------------------------------------------------------------------
void MonaController::PrintSelf(ostream& os, vtkIndent indent)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Initialized: " << (MonaController::Initialized ? "(yes)" : "(no)") << endl;
}

MonaCommunicator* MonaController::WorldRMICommunicator = 0;

//----------------------------------------------------------------------------
void MonaController::TriggerRMIInternal(
  int remoteProcessId, void* arg, int argLength, int rmiTag, bool propagate)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  MonaCommunicator* mpiComm = MonaCommunicator::SafeDownCast(this->RMICommunicator);
  int use_ssend = mpiComm->GetUseSsend();
  if (MonaController::UseSsendForRMI == 1 && use_ssend == 0)
  {
    mpiComm->SetUseSsend(1);
  }

  this->Superclass::TriggerRMIInternal(remoteProcessId, arg, argLength, rmiTag, propagate);

  if (MonaController::UseSsendForRMI == 1 && use_ssend == 0)
  {
    mpiComm->SetUseSsend(0);
  }
}

//----------------------------------------------------------------------------
void MonaController::Initialize()
{
  DEBUG( "replaced, monaContorller call function: " << __FUNCTION__ );
  this->Initialize(0, 0, 1);
}

//----------------------------------------------------------------------------
/*
void MonaController::Initialize(int* argc, char*** argv, int initializedExternally)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  if (MonaController::Initialized)
  {
    vtkWarningMacro("Already initialized.");
    return;
  }

  // Can be done once in the program.
  MonaController::Initialized = 1;
  if (initializedExternally == 0)
  {
    MPI_Init(argc, argv);
  }
  this->InitializeCommunicator(MonaCommunicator::GetWorldCommunicator());

  int tmp;
  MPI_Get_processor_name(ProcessorName, &tmp);
  // Make a copy of MPI_COMM_WORLD creating a new context.
  // This is used in the creating of the communicators after
  // Initialize() has been called. It has to be done here
  // because for this to work, all processes have to call
  // MPI_Comm_dup and this is the only method which is
  // guaranteed to be called by all processes.
  MonaController::WorldRMICommunicator = MonaCommunicator::New();
  MonaController::WorldRMICommunicator->Duplicate((MonaCommunicator*)this->Communicator);
  this->RMICommunicator = MonaController::WorldRMICommunicator;
  // Since we use Delete to get rid of the reference, we should use nullptr to register.
  this->RMICommunicator->Register(nullptr);

  this->Modified();
}
*/

//create the mona internally
void MonaController::Initialize(int* argc, char*** argv, int initializedExternally)
{
  DEBUG( "replaced, monaContorller call function: " << __FUNCTION__ );
  if (MonaController::Initialized)
  {
    vtkWarningMacro("Already initialized.");
    return;
  }

  // Can be done once in the program.
  MonaController::Initialized = 1;
  if (initializedExternally == 0)
  {
    // the mpi is needed to bootstrap the mona
    MPI_Init(argc, argv);
  }
  //the actual operations to create the mona comm
  //and register it into the controller
  this->InitializeCommunicator(MonaCommunicator::GetWorldCommunicator());
  // try to remove this when MPI is not used when init the colza
  // int tmp;
  // MPI_Get_processor_name(ProcessorName, &tmp);
  // Make a copy of MPI_COMM_WORLD creating a new context.
  // This is used in the creating of the communicators after
  // Initialize() has been called. It has to be done here
  // because for this to work, all processes have to call
  // MPI_Comm_dup and this is the only method which is
  // guaranteed to be called by all processes.
  
  // TODO? set it as GetWorldCommunicator or null currently
  // consider this thing when the RMICommunicator will be used actually
  // create the WorldRMICommunicator
  MonaController::WorldRMICommunicator = NULL;

  // set the necessary info into the new communicator
  // ((MonaCommunicator*)(this->Communicator))->Duplicate(MonaController::WorldRMICommunicator);

  this->RMICommunicator = NULL;
  // Since we use Delete to get rid of the reference, we should use nullptr to
  // register.
  // TODO, consider RMICommunicator in future
  // this->RMICommunicator->Register(nullptr);
  this->Modified();
}

//use the mona created outside by the caller
void MonaController::Initialize(int* argc, char*** argv, int initializedExternally, mona_comm_t mona_comm)
{
  DEBUG( "replaced, monaContorller call function: " << __FUNCTION__ );
  
  //this may initilized multiple times
  if (MonaController::Initialized)
  {
    vtkWarningMacro("Already initialized.");
    return;
  }

  // Can be done once in the program.
  MonaController::Initialized = 1;
  if (initializedExternally == 0)
  {
    // the mpi is needed to bootstrap the mona
    MPI_Init(argc, argv);
  }
  //the actual operations to create the mona comm
  //and register it into the controller
  this->InitializeCommunicator(MonaCommunicator::GetWorldCommunicatorByMona(mona_comm));
  // try to remove this when MPI is not used when init the colza
  // int tmp;
  // MPI_Get_processor_name(ProcessorName, &tmp);
  // Make a copy of MPI_COMM_WORLD creating a new context.
  // This is used in the creating of the communicators after
  // Initialize() has been called. It has to be done here
  // because for this to work, all processes have to call
  // MPI_Comm_dup and this is the only method which is
  // guaranteed to be called by all processes.
  
  // TODO? set it as GetWorldCommunicator or null currently
  // consider this thing when the RMICommunicator will be used actually
  // create the WorldRMICommunicator
  MonaController::WorldRMICommunicator = NULL;

  // set the necessary info into the new communicator
  // ((MonaCommunicator*)(this->Communicator))->Duplicate(MonaController::WorldRMICommunicator);

  this->RMICommunicator = NULL;
  // Since we use Delete to get rid of the reference, we should use nullptr to
  // register.
  // TODO, consider RMICommunicator in future
  // this->RMICommunicator->Register(nullptr);
  this->Modified();
}


const char* MonaController::GetProcessorName()
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  return ProcessorName;
}

// Good-bye world
// There should be no MPI calls after this.
// (Except maybe MPI_XXX_free()) unless finalized externally.
void MonaController::Finalize(int finalizedExternally)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  if (MonaController::Initialized)
  {
    //MonaController::WorldRMICommunicator->Delete();
    //MonaController::WorldRMICommunicator = 0;
    if(MonaCommunicator::WorldCommunicator!=0){
    MonaCommunicator::WorldCommunicator->Delete();
    MonaCommunicator::WorldCommunicator = 0;
    }
    this->SetCommunicator(0);
    if (this->RMICommunicator)
    {
      this->RMICommunicator->Delete();
      this->RMICommunicator = 0;
    }
    if (finalizedExternally == 0)
    {
      MPI_Finalize();
    }
    MonaController::Initialized = 0;
    this->Modified();
  }
}

// Called by SetCommunicator and constructor. It frees but does
// not set RMIHandle (which should not be set by using MPI_Comm_dup
// during construction).
void MonaController::InitializeCommunicator(MonaCommunicator* comm)
{
  DEBUG( "replaced, monaContorller call function: " << __FUNCTION__ );
  if (this->Communicator != comm)
  {
    if (this->Communicator != 0)
    {
      this->Communicator->UnRegister(this);
    }
    this->Communicator = comm;
    if (this->Communicator != 0)
    {
      this->Communicator->Register(this);
    }

    this->Modified();
  }
}

// Delete the previous RMI communicator and creates a new one
// by duplicating the user communicator.

void MonaController::InitializeRMICommunicator()
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  if (this->RMICommunicator)
  {
    this->RMICommunicator->Delete();
    this->RMICommunicator = 0;
  }
  if (this->Communicator)
  {
    this->RMICommunicator = MonaCommunicator::New();
    ((MonaCommunicator*)this->RMICommunicator)->Duplicate((MonaCommunicator*)this->Communicator);
  }
}

/*
void MonaController::InitializeRMICommunicator()
{
  DEBUG( "replaced, monaContorller call function: " << __FUNCTION__ );
  if (this->RMICommunicator)
  {
    this->RMICommunicator->Delete();
    this->RMICommunicator = 0;
  }
  if (this->Communicator)
  {
    this->RMICommunicator = MonaCommunicator::New();
    ((MonaCommunicator*)(this->Communicator))
      ->Duplicate((MonaCommunicator*)this->RMICommunicator);
  }
}
*/
/*
void MonaController::SetCommunicator(MonaCommunicator* comm)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  this->InitializeCommunicator(comm);
  this->InitializeRMICommunicator();
}
*/

void MonaController::SetCommunicator(MonaCommunicator* comm)
{
  DEBUG( "replaced, monaContorller call function: " << __FUNCTION__ );
  //TODO free the old communicator
  this->InitializeCommunicator(comm);
  //TODO consider RMI communication in future
  //this->InitializeRMICommunicator();
}

//----------------------------------------------------------------------------
// Execute the method set as the SingleMethod.
void MonaController::SingleMethodExecute()
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  if (!MonaController::Initialized)
  {
    vtkWarningMacro("MPI has to be initialized first.");
    return;
  }

  if (this->GetLocalProcessId() < this->GetNumberOfProcesses())
  {
    if (this->SingleMethod)
    {
      vtkMultiProcessController::SetGlobalController(this);
      (this->SingleMethod)(this, this->SingleData);
    }
    else
    {
      vtkWarningMacro("SingleMethod not set.");
    }
  }
}

//----------------------------------------------------------------------------
// Execute the methods set as the MultipleMethods.
void MonaController::MultipleMethodExecute()
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  if (!MonaController::Initialized)
  {
    vtkWarningMacro("MPI has to be initialized first.");
    return;
  }

  int i = this->GetLocalProcessId();

  if (i < this->GetNumberOfProcesses())
  {
    vtkProcessFunctionType multipleMethod;
    void* multipleData;
    this->GetMultipleMethod(i, multipleMethod, multipleData);
    if (multipleMethod)
    {
      vtkMultiProcessController::SetGlobalController(this);
      (multipleMethod)(this, multipleData);
    }
    else
    {
      vtkWarningMacro("MultipleMethod " << i << " not set.");
    }
  }
}

char* MonaController::ErrorString(int err)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  char* buffer = new char[MPI_MAX_ERROR_STRING];
  int resLen;
  MPI_Error_string(err, buffer, &resLen);
  return buffer;
}

//-----------------------------------------------------------------------------
MonaController* MonaController::CreateSubController(vtkProcessGroup* group)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  VTK_CREATE(MonaCommunicator, subcomm);

  if (!subcomm->Initialize(group))
  {
    return nullptr;
  }

  // MPI is kind of funny in that in order to create a communicator from a
  // subgroup of another communicator, it is a collective operation involving
  // all of the processes in the original communicator, not just those belonging
  // to the group.  In any process not part of the group, the communicator is
  // created with MPI_COMM_NULL.  Check for that and return nullptr ourselves,
  // which is not really an error condition.
  if (*(subcomm->GetMPIComm()->Handle) == MPI_COMM_NULL)
  {
    return nullptr;
  }

  MonaController* controller = MonaController::New();
  controller->SetCommunicator(subcomm);
  return controller;
}

//-----------------------------------------------------------------------------
MonaController* MonaController::PartitionController(int localColor, int localKey)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  VTK_CREATE(MonaCommunicator, subcomm);

  if (!subcomm->SplitInitialize(this->Communicator, localColor, localKey))
  {
    return nullptr;
  }

  MonaController* controller = MonaController::New();
  controller->SetCommunicator(subcomm);
  return controller;
}

//-----------------------------------------------------------------------------
int MonaController::WaitSome(
  const int count, MonaCommunicator::Request rqsts[], vtkIntArray* completed)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  assert("pre: completed array is nullptr!" && (completed != nullptr));

  // Allocate set of completed requests
  completed->SetNumberOfComponents(1);
  completed->SetNumberOfTuples(count);

  // Downcast to MPI communicator
  MonaCommunicator* myMPICommunicator = (MonaCommunicator*)this->Communicator;

  // Delegate to MPI communicator
  int N = 0;
  int rc = myMPICommunicator->WaitSome(count, rqsts, N, completed->GetPointer(0));
  assert("post: Number of completed requests must N > 0" && (N > 0) && (N < (count - 1)));
  completed->Resize(N);

  return (rc);
}

//-----------------------------------------------------------------------------
bool MonaController::TestAll(const int count, MonaCommunicator::Request requests[])
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  int flag = 0;

  // Downcast to MPI communicator
  MonaCommunicator* myMPICommunicator = (MonaCommunicator*)this->Communicator;

  myMPICommunicator->TestAll(count, requests, flag);
  if (flag)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool MonaController::TestAny(const int count, MonaCommunicator::Request requests[], int& idx)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  int flag = 0;

  // Downcast to MPI communicator
  MonaCommunicator* myMPICommunicator = (MonaCommunicator*)this->Communicator;

  myMPICommunicator->TestAny(count, requests, idx, flag);
  if (flag)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool MonaController::TestSome(
  const int count, MonaCommunicator::Request requests[], vtkIntArray* completed)
{
  DEBUG( "monaContorller call function: " << __FUNCTION__ );
  assert("pre: completed array is nullptr" && (completed != nullptr));

  // Allocate set of completed requests
  completed->SetNumberOfComponents(1);
  completed->SetNumberOfTuples(count);

  // Downcast to MPI communicator
  MonaCommunicator* myMPICommunicator = (MonaCommunicator*)this->Communicator;

  int N = 0;
  myMPICommunicator->TestSome(count, requests, N, completed->GetPointer(0));
  assert("post: Number of completed requests must N > 0" && (N > 0) && (N < (count - 1)));

  if (N > 0)
  {
    completed->Resize(N);
    return true;
  }
  else
  {
    completed->Resize(0);
    return false;
  }
}
