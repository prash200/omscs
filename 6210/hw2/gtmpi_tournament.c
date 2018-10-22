#include <stdlib.h>
#include <mpi.h>
#include "gtmpi.h"

#define WINNER (0)
#define LOSER (1)
#define BYE (2)
#define CHAMPION (3)
#define DROPOUT (4)
#define NONE (5)

typedef struct round
{
  int opponent;
  unsigned char role;
  unsigned char flag;  
} round_t;

static round_t **rounds;
static int num_procs, num_rounds;

static int ceillog2(int val)
{
  int i = 0;

  while (val >> i != 1)
  {
    i++;
  }

  return ((1 << i) == val) ? i : i+1;
}

 void gtmpi_init(int num_threads)
 {
  num_procs = num_threads;
  num_rounds = ceillog2(num_procs) + 1;

  rounds = (round_t **) malloc(num_procs * sizeof(round_t *));
  
  for (int i = 0; i < num_procs; i++)
  {
    rounds[i] = (round_t *) malloc(num_rounds * sizeof(round_t));

    for (int j = 0; j < num_rounds; j++)
    {
      rounds[i][j].flag = 0;
      
      if (j > 0)
      {
        if ((i % (1 << j) == 0) && (i + (1 << (j-1)) < num_procs) && ((1 << j) < num_procs))
        {
          rounds[i][j].role = WINNER;
        }
        else if ((i % (1 << j) == 0) && (i + (1 << (j-1)) >= num_procs))
        {
          rounds[i][j].role = BYE;
        }
        else if ((i % (1 << j)) == (1 << (j-1)))
        {
          rounds[i][j].role = LOSER;
        }
        else if ((i == 0) && ((1 << j) >= num_procs))
        {
          rounds[i][j].role = CHAMPION;
        }
        else
        {
          rounds[i][j].role = NONE;
        }
      }
      else
      {
        rounds[i][j].role = DROPOUT;
      }

      if (rounds[i][j].role == LOSER)
      {
        rounds[i][j].opponent = i - (1 << (j-1));
      }
      else if (rounds[i][j].role == WINNER || 
        rounds[i][j].role == CHAMPION)
      {
        rounds[i][j].opponent = i + (1 << (j-1));
      }
    }
  }
}

void gtmpi_barrier()
{
  static char sense = 1;

  MPI_Status stat;
  unsigned int round = 1;
  int vpid;
  char exit_loop = 0;

  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);

  while (!exit_loop)
  {
    switch (rounds[vpid][round].role)
    {
      case LOSER:
        MPI_Send(&sense, 1, MPI_CHAR, rounds[vpid][round].opponent, 0, MPI_COMM_WORLD);
        MPI_Recv(&rounds[vpid][round].flag, 1, MPI_CHAR, rounds[vpid][round].opponent, 0, MPI_COMM_WORLD, &stat);
        exit_loop = 1;
        break;

      case WINNER:
        MPI_Recv(&rounds[vpid][round].flag, 1, MPI_CHAR, rounds[vpid][round].opponent, 0, MPI_COMM_WORLD, &stat);
        break;

      case BYE:
        break;

      case CHAMPION:
        MPI_Recv(&rounds[vpid][round].flag, 1, MPI_CHAR, rounds[vpid][round].opponent, 0, MPI_COMM_WORLD, &stat);
        MPI_Send(&sense, 1, MPI_CHAR, rounds[vpid][round].opponent, 0, MPI_COMM_WORLD);
        exit_loop = 1;
        break;

      case DROPOUT:
        break;

      default:
        break;
    }

    if (!exit_loop)
    {
      round++;
    }
  }

  exit_loop = 0;

  while (!exit_loop)
  {
    --round;

    switch (rounds[vpid][round].role)
    {
      case LOSER:
        break;

      case WINNER:
        MPI_Send(&sense, 1, MPI_CHAR, rounds[vpid][round].opponent, 0, MPI_COMM_WORLD);
        break;

      case BYE:
        break;

      case CHAMPION:
        break;

      case DROPOUT:
        exit_loop = 1;
        break;

      default:
        break;
    }
  }

  sense = !sense;
}

void gtmpi_finalize()
{
  int i;
  
  for (i = 0; i < num_procs; i++)
  {
    free(rounds[i]);
  }

  free(rounds);
}