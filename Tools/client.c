#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "client.h"

void clearValue(Value v) {
  switch(v.type) {
    case ERROR:
    case STATUS:
    case STRING:
      if (v.x.string != (char *)NULL) {
	free(v.x.string);
	v.x.string=(char *)NULL;
      }
      break;
    case BOOLEAN:
    case INTEGER:
      v.x.integer=0;
      break;
    case POINTER:
      if(v.x.pointer != (void *)NULL) {
	free(v.x.pointer);
      }
    case NONE:
      break;
    default:
      fprintf(stderr,"Unrecognized type\n");
      exit(3);
  }
  v.type=NONE;
}
//
// Take a redis command (e.g. LPUSH CHAN Value) and
// Create a proticol compliant packet
//
// NOTE: Does not checking that the command is valid.
//
char *redisCommand(int fd,char *cmd) {
  char *token;
  size_t len;
  int tokenCount=0;
  int redisLength=0;
  char *tokens[12];
  char redisPacket[1024];
  char buildBuffer[255];
  int i=0;
  int res=0;
  bzero( &buildBuffer,255);

  token=strtok(cmd," ");
  while( (char *)NULL != token){
    if( (char *)NULL != token) {
      len=strlen(token);

      redisLength += len;
      //      printf("Length=%d\n",len);
      //      printf("Token =>%s<\n",token);

      tokens[tokenCount] = token;

      tokenCount++;
      token=strtok(NULL," ");
    }

    tokens[tokenCount] = (char *)NULL;

    redisLength = redisLength + (tokenCount *2) + 1;
    #ifdef DEBUG
    printf("Token Count =%d\n", tokenCount);
    printf("Redis Length=%d\n", redisLength);
    #endif
  }


  /*
   *  redisPacket = (char *)malloc( redisLength);
   *
   *  if( (char *) NULL == redisPacket ) {
   *    fprintf(stderr,"redisCommand:Malloc failed\n");
   *    exit(2);
}
*/

  bzero(redisPacket, redisLength);
  sprintf(redisPacket,"*%d\r\n", tokenCount);
  for( i=0; i<tokenCount; i++) {
    sprintf(buildBuffer,"$%d\r\n%s\r\n",strlen(tokens[i]),tokens[i]);
    strcat(redisPacket,buildBuffer);
    //    printf("tokens[%d]=>%s<\n",i,tokens[i]);
  }

  res=write(fd,redisPacket,strlen(redisPacket));

  //  printf("res=%d\n",res);
}

//
// Return > 0 indicates succes, and the value has context dependent meaning.
// < 0 indicates an error
//
Value redisReply(int fd) {
  int res=0;
  char respBuffer[255];
  char *ptr;
  int i=0;
  int dataLength=0;
  int len;

  Value ret;
  ret.type=NONE;
  ret.x.integer=0;
  ret.x.string=(char *)NULL;
  ret.x.pointer=(void *)NULL;


  bzero(respBuffer,255);
  dataLength=read(fd,respBuffer,255);
  respBuffer[dataLength] = (char)NULL;

  #ifdef DEBUG
  printf("==================\n");
  printf("   redisReply\n");
  printf("==================\n");
  printf("Length=%d\n",dataLength);

  for(i=0;i<res;i++) {
    printf("[%d]\t",i);

    if ( respBuffer[i] <= 0x20) {
      printf("0x%02X\n",respBuffer[i]);
    } else {
      printf("%c\n",respBuffer[i]);
    }
  }
  #endif

  switch( respBuffer[0]) {
    case ':': // Integer
      ret.type = INTEGER;
      ptr=(char *)strtok( &respBuffer[1],"\r");
      res=atoi(ptr);
      ret.x.integer=res;
      break;
    case '-':  // Error
      ret.type=ERROR;
      //      printf("ERROR >%s<\n", respBuffer);
      ret.x.string=(char *)malloc( len + 1);
      strncpy(ret.x.string, &respBuffer[1],len );
      break;
    case '+':
      ret.type=STATUS;
      len = strlen( &respBuffer[1])-2;
      //      printf("STATUS >%s<\n", respBuffer);
      //      printf("LEN     %d\n",len);

      ret.x.string=(char *)malloc( len + 1);
      strncpy(ret.x.string, &respBuffer[1],len );
      break;
    case '$':
      ret.type=STRING;
      ptr=(char *)strtok( &respBuffer[1],"\r");
      res=atoi(ptr);

      ptr=(char *)strtok(NULL,"\r");
      ptr++;   // Set past previos tokens \n

      ret.x.string=(char *)malloc( res + 1);
      strncpy(ret.x.string, ptr,res );
      break;
    default:
      break;
  }
  return(ret);
}
//
// TODO Need to add a timeout.  Only wait for specified tome, then return a failure.
//
int redisPing(int ser) {
  char cmdBuffer[255];
  Value v;
  int status=0;

  bzero(cmdBuffer,255);
  strcpy(cmdBuffer,(char *)"PING");
  redisCommand(ser,(char *)cmdBuffer);
  v=redisReply(ser);

  if( v.type== STATUS) {
    if ( !strcmp(v.x.string,(char *)"PONG")) {
      status = 1;
    } else {
      status = 0;
    }
  }

  clearValue(v);
  return(status);
}
//
// qname - name of Q
// data  - Data to send (array of unsigned chars)
// len   - Length of data
//
// Returns length 0f data sent or an error (<0)
//
int enQueue(int ser, char *qname, char *data, int len) {
  char cmdBuffer[255];
  Value v;
  int status;

  sprintf(cmdBuffer,"LPUSH %s %s", qname,data);
  redisCommand(ser,(char *)cmdBuffer);
  v=redisReply(ser);
  //
  // Check type for error
  //

  if(v.type == ERROR) {
    fprintf(stderr,"ERROR: enQueue ><\n");
  }
  clearValue(v);
  return(status);
}
//
// qname - name of Q
// data  - Buffer to hold recieved Data (array of unsigned chars)
// len   - Size of buffer
//
// Return length of data recieved.
//
char *deQueue(int ser,char *qname) {
  char cmdBuffer[255];
  Value v;

  sprintf(cmdBuffer,(char *)"RPOP %s", qname);
  redisCommand(ser,(char *)cmdBuffer);
  v=redisReply(ser);

  return(v.x.string);
}
//
// qname - name of Q
// data  - Buffer to hold recieved Data (array of unsigned chars)
// len   - Size of buffer
//
// Return length of data recieved.
//
int blockingDeQueue(char *qname, char *data, int len) {
}
