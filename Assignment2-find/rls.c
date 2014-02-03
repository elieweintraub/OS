/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #2: rls.c                                    */
/*                                                                           */
/*  rls -  Lists directory contents while performing a recursive walk of the */
/*         file system beginning at the supplied pathname                    */
/*                                                                           */
/*  USAGE:                                                                   */
/*		   rls  [-u user] [-m mtime] starting_path                           */ 
/*                                                                           */ 
/*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PATH_LEN 1024
#define STMP_SIZE 18
#define P_STR_SIZE 11

// Parses command line arguments
void parse_args(int argc, char *argv[], int *u_flg, int *m_flg, char **path);

// Prints the petinent information when a node in the file system is visited
int visit_node(char *node_name, int u_flg, int m_flg, char *argv[], struct stat *stp);

// Recursively traverses the file system from a given directory
int traverse_fs(char *cur_dir, int u_flg, int m_flg, struct stat *stp, char *argv[]);

//Builds a path from a path prefix and suffix, w/ the result pointed to by path 
int create_path(char *path_prefix, char *path, char*path_suffix, char *argv[]);

// Returns a statically allocated timestamp
char *timestamp(const time_t *t);

// Returns a statically allocated permission string
char *pstring(const struct stat *stp);

// Checks if input string corresponds to an interger
int isint(char *x);


int main(int argc,char *argv[]){
	int u_flg = -1;            // -1 corresponds to no -u flag supplied
	int m_flg = 0;             // 0 corresponds to no -m flag supplied
	char *path = NULL;         // initial path provided by user
	struct stat st;
	int retval = 0;
	
	//Parse command line arguments
	parse_args(argc, argv, &u_flg, &m_flg, &path);
	
	//Remove trailing slash in pathname if supplied
	if (path[strlen(path)-1] == '/') path[strlen(path)-1] = '\0';
	
	//Visit initial node
	if (visit_node(path, u_flg, m_flg, argv, &st)) return -1;
	
	//Recursively visit further nodes if initial node is a directory
	if (S_ISDIR(st.st_mode)){ 
		return traverse_fs(path, u_flg, m_flg, &st, argv);
	}
	return 0;
}	

// PARSE_ARGS - Parses command line arguments	
void parse_args(int argc, char *argv[], int *u_flg, int *m_flg, char **path){	
	int opt, i;
	struct passwd *pwd;
	//Parse options
	while ((opt = getopt(argc,argv,"u:m:h"))!=-1){
		switch(opt){
			case 'u':
				if (isint(optarg)==1){ // non-negative int, assume valid uid
					*u_flg = atoi(optarg);
				}	
				else { // interpret as username
					if ((pwd = getpwnam(optarg)) == NULL){
						fprintf(stderr, "%s: no uid corresponding to %s\n",
						        argv[0], optarg);
						exit(-1);
					}
					*u_flg = (int)pwd->pw_uid;
				}	
				break;
			case 'm':
				if (!isint(optarg) || (*m_flg=atoi(optarg)) == 0){
					fprintf(stderr, "%s: -m argument must be a non-zero "
           					"integer\n", argv[0]);
					exit(-1);
				}	
				break;
			case 'h':	
				printf("USAGE: %s [-u user] [-m mtime] starting_path\n", argv[0]); 
				exit(0);
				break;
			default: /* '?' */
				fprintf(stderr, "Try '%s -h' for more information\n", argv[0]);
				exit(-1);
        }
    }
	//Parse starting_path argument
	if (optind >= argc){ // missing starting path argument
		fprintf(stderr, "%s: missing starting_path\n"
				"Try '%s -h' for more information\n", argv[0], argv[0]);
		exit(-1);
	}	
	else if (optind < argc-1){ // multiple starting paths supplied
			fprintf(stderr, "%s: only a single starting_path allowed\n"
				"Try '%s -h' for more information\n", argv[0], argv[0]);
		exit(-1);
	}	
	else { //optind==argc-1  A single starting path was supplied
		*path = argv[optind];	
	}
}

// TRAVERSE_FS - Recursively traverses the file system from a given directory
int traverse_fs(char *cur_dir, int u_flg, int m_flg, struct stat *stp, char *argv[]){
	DIR *dirp;
	struct dirent *de;
	char path[MAX_PATH_LEN];
	int tmp, retval = 0;  // retval is an error count (retval= - # of errors)
	
	//Open directory stream
	if ((dirp = opendir(cur_dir)) == NULL){
		fprintf(stderr, "%s: can not open directory %s: %s\n", argv[0],
				cur_dir, strerror(errno));
		errno = 0; // reset for potential later use		
		return -1;
	}
	//Read directory entries from stream
	while ((de = readdir(dirp))!=NULL){
		if(strcmp(de->d_name, ".")!=0 &&  strcmp(de->d_name, "..")!=0){
			//Create path to directory entry
			if (!create_path(cur_dir, path, de->d_name, argv)){ 
				//Visit corresponding node
				tmp = visit_node(path, u_flg, m_flg, argv, stp);
				retval+=tmp;
				//If node is dir and no error, recursively traverse its entries
				if (de->d_type == DT_DIR && !tmp){ 
					retval+=traverse_fs(path, u_flg, m_flg, stp, argv);
				}
			}
			else{ //error, pathname will overflow buffer
				retval-=1;
			}
		}
	}
	if (errno){ // Some error occurred while reading from the directory stream
		fprintf(stderr, "%s: error occurred while reading directory %s: %s\n",
				argv[0], cur_dir, strerror(errno));
		errno = 0; // reset for potential later use			
		return retval-1;
	}
	//Close directory stream
	closedir(dirp);
	return retval;
}

// VISIT_NODE - Prints the information when a node in the file system is visited
int visit_node(char *node_name, int u_flg, int m_flg, char *argv[], struct stat* stp){
	struct passwd *pwd;
	struct group *grp;
	char *p_str = NULL;
	char *uname = NULL;
	char *gname = NULL;
	char *lname = NULL;
	ssize_t lnk_sz;
	char *t;  
	
	//stat the file
	if (lstat(node_name, stp)){
		fprintf(stderr, "%s: error using stat on %s: %s\n",
		        argv[0], node_name, strerror(errno));
		errno = 0; // reset for potential later use			
		return -1;
	}
	//If u_flg set, check uid
	if (u_flg!=-1 && u_flg!=stp->st_uid) return 0;	
	//If m_flg set, check file modification time
	if ((m_flg > 0) && (time(NULL) - stp->st_mtime) < m_flg) return 0;
	if ((m_flg < 0) && (stp->st_mtime - time(NULL)) < m_flg) return 0;
	//get permission string
	p_str = pstring(stp);
	//get username
	if (((pwd = getpwuid(stp->st_uid)) == NULL) && errno){ //Some error occurred
		fprintf(stderr, "%s: error accessing information corresponding to uid %d,"
				"owner of %s: %s\n", argv[0], stp->st_uid, node_name, strerror(errno));
		errno = 0; // reset for potential later use			
		return -1;
	}
	else if (pwd == NULL){;} // No entry found, but no error - do nothing
	else { // entry found
		uname = pwd->pw_name;
	}
	//get group name
	if (((grp = getgrgid(stp->st_gid)) == NULL) && errno){ //Some error occurred
		fprintf(stderr, "%s: error accessing information corresponding to gid %d,"
				"owner of %s: %s\n", argv[0], stp->st_gid, node_name, strerror(errno));
		errno = 0; // reset for potential later use			
		return -1;
	}
	else if (grp == NULL){;} // No entry found, but no error - do nothing
	else { // entry found
		gname = grp->gr_name;
	}
	//get timestamp
	if ((t = timestamp(&(stp->st_mtime))) == NULL){ // Some error occurred
		fprintf(stderr, "%s: error producing timestamp of %s: %s\n",
		        argv[0], node_name, strerror(errno));
		errno = 0; // reset for potential later use			
		return -1;
	}
	//get linkname
	if (S_ISLNK(stp->st_mode)){
		lname = malloc(stp->st_size+1);
		if (lname == NULL) {
			fprintf(stderr, "%s: error producing symlink value: "
					"insufficient memory\n",argv[0]);
			errno = 0; // reset for potential later use	
			return -1;		
		}
		lnk_sz = readlink(node_name,lname,stp->st_size + 1);
		if (lnk_sz < 0) {
			fprintf(stderr, "%s: error getting symlink value for %s: %s\n",
					argv[0],node_name,strerror(errno));
			errno = 0; // reset for potential later use	
			return -1;	
		}
		else if (lnk_sz > stp->st_size) {
			fprintf(stderr, "%s: symlink increased in size between lstat() "
					"and readlink()\n");
			return -1;
		}
		lname[stp->st_size] = '\0';
	}	
	//print the complete string
	printf("%04lX/%04lX  %s %4d  ", stp->st_dev, stp->st_ino, p_str, stp->st_nlink);
	(uname)?printf("%-8s ", uname):printf("%-8d ", stp->st_uid); 
	(gname)?printf("%-8s ", gname):printf("%-8d ", stp->st_gid); 
	(S_ISBLK(stp->st_mode)||S_ISCHR(stp->st_mode))?printf("%p",stp->st_rdev):
	                                               printf("%6d",stp->st_size);
	printf("  %s  %s", t, node_name);
	(lname)?printf(" -> %s\n",lname):printf("\n");	
	return 0;
}

// TIMESTAMP - Returns a statically allocated timestamp. Returns NULL on error
char *timestamp(const time_t *t){
	struct tm *Tm;
	int m,h,d,y;
	char mn[4];
	static char time[STMP_SIZE];
	char *months[] = {"Jan","Feb","Mar","Apr","May","Jun",
	                  "Jul","Aug","Sep","Oct","Nov","Dec"};
	
	if ((Tm = localtime(t)) == NULL) return NULL;
	d = Tm->tm_mday;
	y = Tm->tm_year+1900;
	h = Tm->tm_hour;
	m = Tm->tm_min;
	strcpy(mn, months[Tm->tm_mon]);		
	snprintf(time, STMP_SIZE, "%s %02d %4d %02d:%02d", mn, d, y, h, m);
	return time;
}

// PSTRING - Returns a statically allocated permission string
char *pstring(const struct stat *stp){
	static char p_str[P_STR_SIZE];
	char type[] = {'?', 'p', 'c', '?', 'd', '?', 'b', '?', '-',
	               '?', 'l', '?', 's', '?', '?', '?', '?'};
	char *rwx[] = {"---","--x","-w-","-wx","r--","r-x","rw-","rwx"};
	mode_t mode = stp->st_mode;
	
	p_str[0] = type[(mode & S_IFMT)>>12]; //filetype
	strcpy(&p_str[1], rwx[(mode & S_IRWXU)>>6]); // owner permissions
    strcpy(&p_str[4], rwx[(mode & S_IRWXG)>>3]); // group permissions
    strcpy(&p_str[7], rwx[mode & S_IRWXO]); // other permissions
	if (mode & S_ISUID) p_str[3] = (mode & S_IXUSR) ? 's' : 'S'; // setuid
    if (mode & S_ISGID) p_str[6] = (mode & S_IXGRP) ? 's' : 'S'; // setgid
    if (mode & S_ISVTX) p_str[9] = (mode & S_IXOTH) ? 't' : 'T'; // sticky bit
    p_str[10] = '\0';
    return p_str;
}	

// CREATE_PATH - Builds a path from a path_prefix and path_suffix
int create_path(char *path_prefix, char *path, char*path_suffix, char *argv[]){	
	if ((strlen(path_prefix)+strlen(path_suffix)+1)< MAX_PATH_LEN){
		strcpy(path, path_prefix);
		strcat(path, "/");
		strcat(path, path_suffix);
		return 0;
	}	
	else{
		fprintf(stderr, "%s: pathname %s/%s exceeds maximum path length  of %d\n",
		        argv[0],path_prefix, path_suffix, MAX_PATH_LEN );
		return -1;
	}
}

// ISINT - A helper function that checks if input string is an integer.
//         If string is negative, return -1. ("-0" is considered neg.)
//         If string is non-negative, return 1. Otherwise,return 0. 
int isint(char *x){
	int i, neg_flag = 0;
	if (x[0] == '-') {neg_flag = 1; x++;}
	for (i = 0; i < strlen(x); i++) if (!isdigit(x[i])) break;	
	if (i!=strlen(x)) return 0;	
	return neg_flag?-1:1; 
}	