#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>     // for dirname()/basename()
#include <time.h> 

#define MAX 256
#define PORT 44443

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

char gpath[128];
char line[MAX], ans[MAX], *command[10];
int n;
char cwd[MAX]; 

struct sockaddr_in saddr; 
int sfd;

int ls_file(char *fname)
{
  struct stat fstat, *sp;
  int r, i;
  char ftime[64];
  sp = &fstat;
  if ((r = lstat(fname, &fstat)) < 0)
  {
    printf("can't stat %s\n", fname);
    exit(1);
  }
  if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
    printf("%c",'-');
  if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    printf("%c", 'd');
  if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
    printf("%c",'l');
  for (i = 8; i >= 0; i--)
  {
    if (sp->st_mode & (1 << i)) // print r|w|x
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]); // or print -
  }
  printf("%4d ", sp->st_nlink); // link count
  printf("%4d ", sp->st_gid);   // gid
  printf("%4d ", sp->st_uid);   // uid
  printf("%8d ", sp->st_size);  // file size
  // print time
  strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
  ftime[strlen(ftime) - 1] = 0;        // kill \n at end
  printf("%s ", ftime);
  // print name
  printf("%s", basename(fname)); // print file basename
  // print -> linkname if symbolic file
  if ((sp->st_mode & 0xF000) == 0xA000)
  {
    char *linkname = "";
    readlink(linkname, fname, MAX);
    // use readlink() to read linkname
    printf(" -> %s", linkname); // print linked name
  }
  printf("\n");
}

int ls_dir(char *path)
{
    DIR *myDir = opendir(".");
    struct dirent *now;

    if (!myDir)
    {
      printf("Could not be accessed - error\n");
    }

    else
    {
      while ((now = readdir(myDir)) != NULL)
      {
        //printf("%s\n", now->d_name);
        ls_file(now->d_name);
      }

      putchar('\n');
      closedir(myDir);
    }
}

void receiveFile(int sock)
{
  char trans[MAX];
  int x = 0;
  FILE *myFile = fopen(command[1], "w");

  while (1) //strcmp(trans, "}//\n") != 0)
  {
     x = read(sock, trans, MAX); //recv(sock, trans, MAX, 0);

    if (strcmp(trans, "`") == 0)
    {
       break;
    }

    if (x <= 0)
      break;
      //return;

    fprintf(myFile, "%s", trans);
    bzero(trans, MAX);
  }

  return;
}

void doCat()
{
    FILE *myFile = fopen(command[1], "r");
    char trans[MAX] = "";

    while (fgets(trans, MAX, myFile) != 0) // SEGFAULT LINE
    {
        printf("%s", trans);
    }

    putchar('\n');
}

void fileTransfer(int sock)
{
  FILE *myFile = fopen(command[1], "r");
  char trans[MAX] = "";

  // printf("HELLO: %s\n", command[1]);

  while (fgets(trans, MAX, myFile) != 0) // SEGFAULT LINE
  {
    // printf("trans: %s\n", trans);
    send(sock, trans, MAX, 0);
    bzero(trans, MAX);
  }

    //close (sock);

    send(sock, "`", MAX, 0);

}

int checkLocal(char cmd[])
{
    if (cmd[0] == 'l' && (strcmp("ls\n", cmd) != 0))
    {
        return 1;
    }

    return 0;
}

int checkFile(char cmd[])
{
    tokenize(cmd);

    if (strcmp(command[0], "put") == 0)
    {
        return 2;
    }

    if (strcmp(command[0], "get") == 0)
    {
        return 3;
    }

    if (strcmp(command[0], "ls\n") == 0)
    {
      return 4;
    }

    return 0;
}


int getCase()
{

    // printf("COMMAND[0] = %s\n", command[0]);

    if (strcmp("lmkdir", command[0]) == 0)
    {
        return 1;
    }

    else if (strcmp("lrmdir", command[0]) == 0)
    {
        return 2;
    }

    else if (strcmp("lrm", command[0]) == 0)
    {
        return 3;
    }

    else if (strcmp("lcreat", command[0]) == 0)
    {
        return 4;
    }

    else if (strcmp("lls", command[0]) == 0)
    {
       // printf("here\n");
        return 5;
    }

    else if (strcmp("lcat", command[0]) == 0)
    {
        return 6;
    }

    else if (strcmp("lput", command[0]) == 0)
    {
        return 7;
    }

    else if (strcmp("lpwd", command[0]) == 0)
    {
        return 8;
    }

    else if (strcmp("lcd", command[0]) == 0)
    {
        return 9;
    }

    else 
    {
        return 0;
    }
}

int tokenize(char *pathname) // YOU have done this in LAB2
{                                  // YOU better know how to apply it from now on
  char *s;
  strcpy(gpath, pathname); // copy into global gpath[]
  s = strtok(gpath, " ");
  int n = 0;
  while (s)
  {
    // printf("%s", name[n]);
    command[n] = s; // token string pointers
    s = strtok(0, "\n");
    ++n;
  }
  command[n] = 0; // name[n] = NULL pointer
}


void processCommand(char cmd[], int argc, char *argv[])
{
        int r = 0;

  // printf("%s\n", cmd);

    tokenize(cmd);

    strcat(cwd, "/");

    //printf("LOOK HERE : %s\n", cwd);

    if (command[1])
        strcat(cwd, command[1]);

    //printf("LOOK HERE2 : %s\n", cwd);

    int h = getCase();

    // strcat();

    switch (h) 
    {
        case 0:
          printf("Error. command not found\n");
          n = -1;
          return;
        case 1:
          printf("mkdir called\n");
          r =  mkdir(cwd, 0755);
          break;
        case 2:
          printf("rmdir called\n");
          r = rmdir(cwd);//, 0755);
          break;
        case 3:
          printf("rm called\n");
          r = unlink(cwd);
          break;
        case 4:
          printf("creat called\n");
          r = creat(cwd, 0655);
          break;
        case 5:
          printf("ls called\n");
/*
          DIR *myDir = opendir(".");
          struct dirent *now;

          if (!myDir)
          {
            printf("Could not be accessed - error\n");
          }

          else
          {
            while ((now = readdir(myDir)) != NULL)
              printf("%s\n", now->d_name);
            putchar('\n');
            closedir(myDir);
          }*/

            struct stat mystat, *sp = &mystat;
    int r;
    char *filename, path[1024], cwd[256];
    filename = "./"; // default to CWD
    if (argc > 1)
      filename = argv[1]; // if specified a filename
    if (r = lstat(filename, sp) < 0)
    {
      printf("no such file %s\n", filename);
      exit(1);
    }
    strcpy(path, filename);
    if (path[0] != '/')
    { // filename is relative : get CWD path
      //8.6 The stat Systen Call 257 
      getcwd(cwd, 256);
      strcpy(path, cwd);
      strcat(path, "/");
      //strcat(path, filename);
    }

    // printf("%s\n", path);

    if (S_ISDIR(sp->st_mode))
      ls_dir(path);
    else
      ls_file(path);

          break;
          // complicated
        case 6:
          printf("cat called\n");
          doCat();
          break;
        case 8:
          printf("pwd called\n");
          getcwd(cwd, 128);
          printf("%s\n", cwd);
          break;
        case 9:
          printf("cd called\n");
          //printf("%s\n", cwd);
          strcat(cwd, "/");
          strcat(cwd, command[1]);
          //printf("%s\n", cwd);
          r = chdir(cwd);
          break;

    }
}

int main(int argc, char *argv[], char *env[]) 
{ 
    int n; char how[64];
    int i;

    printf("1. create a socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sfd < 0) 
    { 
        printf("socket creation failed\n"); 
        exit(0); 
    }
    
    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr)); 
    saddr.sin_family = AF_INET; 
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    saddr.sin_port = htons(PORT); 
  
    printf("3. connect to server\n");
    if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 

    printf("********  processing loop  *********\n");
    while (1)
    {
        chroot(getcwd(cwd, 128));
      printf("input a line : ");
      bzero(line, MAX);                // zero out line[ ]
      fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

    // INSERT IF FOR LOCAL COMMANDS

      int x = checkLocal(line);  

      int y = checkFile(line);


      line[strlen(line)-1] = 0;        // kill \n at end
      if (line[0]==0 || strcmp(line, "exit") == 0)                  // exit if NULL line
         exit(0);

      // Send ENTIRE line to server
      n = write(sfd, line, MAX);
      printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

      if (y == 2)
      {
          fileTransfer(sfd);
      }

      if (y == 3)
      {
          receiveFile(sfd);
      }

      if (y == 4)
      {
        // printf("HEY\n");
        char buf[MAX] = "";
        while (strcmp(buf, "`") != 0)
        {
          read(sfd, buf, MAX);

          if (strcmp(buf, "`") != 0)
          {
          printf("%s\n", buf);
          }
        }
      }

    if (x == 1)
    {
        //printf("HERE\n");
        processCommand(line, argc, argv);
    }

  
      if (y != 3 && y != 4)
      {
      // Read a line from sock and show it
      n = read(sfd, ans, MAX);
      printf("client: read  n=%d bytes; echo=(%s)\n",n, ans);
      }
    }
  //}
}//
