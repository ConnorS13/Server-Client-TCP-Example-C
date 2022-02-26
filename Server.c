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
#include <libgen.h>
#include <time.h>

#define MAX 256
#define PORT 44443

int n;

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

char gpath[128]; // i added
char ans[MAX];
char line[MAX];
char *command[MAX];     // i added
char cwd[MAX], vr[MAX]; // i added

int ls_file(char *fname, int sock)
{
  struct stat fstat, *sp;
  char stringSent[256] = "", buf[10] = "";
  int r, i;
  char ftime[64];
  sp = &fstat;

  if ((r = lstat(fname, &fstat)) < 0)
  {
    printf("can't stat %s\n", fname);
    exit(1);
  }

  // printf("HEY\n");

  if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
  {
    strcat(stringSent, "-");
    //printf("%c", '-');
  }

  // printf("HEY\n");

  if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
  {
    // printf("HEY\n");
    strcat(stringSent, "d");
    //printf("%c", 'd');
  }

  if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
  {
    // printf("HEY\n");
    strcat(stringSent, "l");
    //printf("%c", 'l');
  }

  for (i = 8; i >= 0; i--)
  {
    if (sp->st_mode & (1 << i)) // print r|w|x
    {
      buf[0] = t1[i];
      strcat(stringSent, buf);
      //printf("%c", t1[i]);
    }

    else
    {
      buf[0] = t2[i];
      strcat(stringSent, buf);
      //printf("%c", t2[i]); // or print -
    }
  }

  strcat(stringSent, "   ");

  strcpy(buf, "");

  sprintf(buf, "%d", sp->st_nlink);
  strcat(stringSent, buf);
  strcat(stringSent, " ");
  strcpy(buf, "");

  sprintf(buf, "%d", sp->st_gid);
  strcat(stringSent, buf);
  strcat(stringSent, " ");
  strcpy(buf, "");

  sprintf(buf, "%d", sp->st_uid);
  strcat(stringSent, buf);
  strcat(stringSent, " ");
  strcpy(buf, "");

  sprintf(buf, "%d", sp->st_size);
  strcat(stringSent, buf);
  strcat(stringSent, " ");
  //printf("%4d ", sp->st_nlink); // link count
  //printf("%4d ", sp->st_gid);
  strcat(stringSent, "   ");   // gid
  //printf("%4d ", sp->st_uid);   // uid
  //strcat(stringSent, "   ");
  //printf("%8d ", sp->st_size);  // file size
  // print time
  strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
  ftime[strlen(ftime) - 1] = 0;        // kill \n at end
  //printf("%s ", ftime);
  strcat(stringSent, ftime);
  strcat(stringSent, " ");
  //  print name
  //printf("%s", basename(fname)); // print file basename
  strcat(stringSent, basename(fname));
  //printf("\n%s\n", stringSent);

  write(sock, stringSent, MAX);

  //  print -> linkname if symbolic file
  if ((sp->st_mode & 0xF000) == 0xA000)
  {
    char *linkname = "";
    readlink(linkname, fname, MAX);
    // use readlink() to read linkname
    printf(" -> %s", linkname); // print linked name
  }

  //printf("\n");
}

int ls_dir(char *path, int sock)
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
      // printf("%s\n", now->d_name);
      ls_file(now->d_name, sock);
    }

    write(sock, "`", MAX);

    putchar('\n');
    closedir(myDir);
  }
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

  // close (sock);

  send(sock, "`", MAX, 0);
}

void receiveFile(int sock)
{
  char trans[MAX];
  int x = 0;
  FILE *myFile = fopen(command[1], "w");

  while (1) // strcmp(trans, "}//\n") != 0)
  {
    x = read(sock, trans, MAX); // recv(sock, trans, MAX, 0);

    if (strcmp(trans, "`") == 0)
    {
      break;
    }

    if (x <= 0)
      break;
    // return;

    fprintf(myFile, "%s", trans);
    bzero(trans, MAX);
  }

  return;
}

int getCase()
{
  if (strcmp("mkdir", command[0]) == 0)
  {
    return 1;
  }

  else if (strcmp("rmdir", command[0]) == 0)
  {
    return 2;
  }

  else if (strcmp("rm", command[0]) == 0)
  {
    return 3;
  }

  else if (strcmp("creat", command[0]) == 0)
  {
    return 4;
  }

  else if (strcmp("ls", command[0]) == 0)
  {
    return 5;
  }

  else if (strcmp("get", command[0]) == 0)
  {
    return 6;
  }

  else if (strcmp("put", command[0]) == 0)
  {
    return 7;
  }

  else if (strcmp("pwd", command[0]) == 0)
  {
    return 8;
  }

  else if (strcmp("cd", command[0]) == 0)
  {
    return 9;
  }

  // else if (strcmp("exit", command[0]) == 0)
  //{
  //   return 9;
  //}

  else
  {
    return 0;
  }
}

int tokenize(char *pathname) // YOU have done this in LAB2
{                            // YOU better know how to apply it from now on
  char *s;
  strcpy(gpath, pathname); // copy into global gpath[]
  s = strtok(gpath, " ");

  if (strcmp(s, "") == 0)
    strcpy(command[0], s);

  else
  {

    int n = 0;
    while (s)
    {
      // printf("%s", name[n]);
      command[n++] = s; // token string pointers
      s = strtok(0, " ");
      //++n;
    }
    command[n] = 0;
  } // name[n] = NULL pointer
}

void processCommand(char *cmd, int sfd, int argc, char *argv[])
{
  int r = 0;

  tokenize(cmd);

  strcat(cwd, "/");

  strcat(cwd, command[1]);

  // printf("LOOK HERE: %s\n", cwd);

  int h = getCase();

  // strcat();

  switch (h)
  {
  case 0:
    // printf("Error. command not found\n");
    n = 256;
    return;
  case 1:
    printf("mkdir called\n");
    r = mkdir(cwd, 0755);
    break;
  case 2:
    printf("rmdir called\n");
    r = rmdir(cwd); //, 0755);
    break;
  case 3:
    printf("rm called\n");
    r = unlink(cwd);
    break;
  case 4:
    printf("creat called\n");
    r = creat(cwd, 0655);
    break;
  case 5:;
    /*printf("ls called\n");

    DIR *myDir = opendir(".");
    struct dirent *now;

    if (!myDir)
    {
      printf("Could not be accessed - error\n");
    }

    else
    {
      while ((now = readdir(myDir)) != NULL)
        printf("%s ", now->d_name);
      putchar('\n');
      closedir(myDir);
    } */

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

    if (cwd[0] != '/')
    { // filename is relative : get CWD path
      // 8.6 The stat Systen Call 257
      getcwd(cwd, 256);
      strcpy(path, cwd);
      strcat(path, "/");
      // strcat(path, filename);
    }

    printf("%s\n", cwd);

    if (S_ISDIR(sp->st_mode))
      ls_dir(path, sfd);
    else
      ls_file(path, sfd);

    break;
    // complicated
  case 6:
    printf("get called\n");
    fileTransfer(sfd);
    break;
  case 7:
    printf("put called\n"); // open(command[1], O_WRONLY | O_CREAT, 0644);
    receiveFile(sfd);
    // printf("put called\n");
    break;
  case 8:
    printf("pwd called\n");
    printf("%s\n", cwd);
    break;
  case 9:
    printf("cd called\n");
    strcat(cwd, "/");
    strcat(cwd, command[1]);
    printf("%s\n", cwd);
    r = chdir(cwd);
    break;
  }
}

int main(int argc, char *argv[])
{
  int sfd, cfd, len;
  struct sockaddr_in saddr, caddr;
  int i, length;

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
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  saddr.sin_port = htons(PORT);

  printf("3. bind socket to server\n");
  if ((bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) == -1)
  {
    printf("socket bind failed\n");
    perror("bind");
    exit(0);
  }

  // Now server is ready to listen and verification
  if ((listen(sfd, 5)) != 0)
  {
    printf("Listen failed\n");
    exit(0);
  }

  int g = chroot(getcwd(cwd, 128));

  // printf("%d\n", g);

  while (1)
  {
    // Try to accept a client connection as descriptor newsock
    length = sizeof(caddr);
    // printf("HEY\n");
    // select();
    cfd = accept(sfd, (struct sockaddr *)&caddr, &length);
    // printf("HEY\n");
    if (cfd < 0)
    {
      printf("server: accept error\n");
      exit(1);
    }

    printf("server: accepted a client connection from\n");
    printf("-----------------------------------------------\n");
    printf("    IP=%s  port=%d\n", "127.0.0.1", ntohs(caddr.sin_port));
    printf("-----------------------------------------------\n");

    // Processing loop
    while (1)
    {
      printf("server ready for next request ....\n");
      n = read(cfd, line, MAX);
      if (n == 0)
      {
        printf("server: client died, server loops\n");
        close(cfd);
        exit(1);
        break;
      }

      // show the line string
      printf("server: read  n=%d bytes; line=[%s]\n", n, line);

      strcat(line, " ECHO");

      // send the echo line to client
      n = write(cfd, line, MAX);

      strcpy(cwd, "");

      vr[0] = '/';

      processCommand(line, cfd, argc, argv);

      if (strcmp(command[0], "exit") == 0) // terminates if first command = exit
        exit(0);

      // printf("WE MADE IT\n\n");

      printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
      printf("server: ready for next request\n");
    }
  }
}
