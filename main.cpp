#include <bits/stdc++.h>
#include <termios.h>
#include <time.h>
#include <sys/types.h>
#include <filesystem>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include<sys/ioctl.h>
#include<stdlib.h>
#include <fcntl.h>


#define MAX 10
using namespace std;

struct termios orig_termios;
struct winsize w;

char cwd[1024];	
char home[1024];

stack<string> fstack;
stack<string> bstack;
vector<dirent*> files;
vector<string> command;
int cursor=0;
string msg="";

bool mode = true;
bool mv = false;
int s=0;
int e=MAX;

DIR *dir;

string getPermission(char *f)
{
	struct stat st;
	stat(f,&st);
	string per;
	per += (S_ISDIR(st.st_mode)) ? "d" : "-";
    	per += (st.st_mode & S_IRUSR) ? "r" : "-";
    	per += (st.st_mode & S_IWUSR) ? "w" : "-";
    	per += (st.st_mode & S_IXUSR) ? "x" : "-";
    	per += (st.st_mode & S_IRGRP) ? "r" : "-";
    	per += (st.st_mode & S_IWGRP) ? "w" : "-";
    	per += (st.st_mode & S_IXGRP) ? "x" : "-";
    	per += (st.st_mode & S_IROTH) ? "r" : "-";
    	per += (st.st_mode & S_IWOTH) ? "w" : "-";
    	per += (st.st_mode & S_IXOTH) ? "x" : "-";

	return per;
}

void disableRawMode()
{
       tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);       
}
void enableRawMode()
{
        tcgetattr(STDIN_FILENO, &orig_termios);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)!=0)
               cout<<"Error!!!!!!!!!"<<endl;
}
void setCursor(int x, int y)
{
	cout<<"\033["<<x<<";"<<y<<"H";
	fflush(stdout);	
}
void printFiles()
{
	cout<<"\033[H\033[J";
	for(int i=s; i<e && i<files.size();i++)
	{
		struct stat info;
		lstat(files[i]->d_name, &info);  
		struct passwd *pw = getpwuid(info.st_uid);
		struct group  *gr = getgrgid(info.st_gid);
		char *str2 = files[i]->d_name;
		string per = getPermission(str2);
		string str = str2;
		if(str.length()>=20)
		{
			str = str.substr(0,17);
			str += "...";
		}
		cout<<setw(20)<<left<<str<<"\t"<<per<<"\t"<<setw(10)<<left<<info.st_size<<"\t"<<pw->pw_name<<"\t"<<gr->gr_name<<"\t\t"<<ctime(&info.st_mtime);
	}
	setCursor(25,0);
	cout<<"-----------------------------------------------------------------------------------------------------"<<endl;
	cout<<"\033[0;31m"<<"MODE : "<<"\033[0m";
	if(mode)
		cout<<"Normal (Press ':' to switch to Command Mode)"<<endl;
	else
		cout<<"Command (Press 'esc' to switch to Normal Mode)"<<endl;
	cout<<"\033[0;31mCurrent Working Directory : \033[0m"<<cwd<<endl;
	cout<<"\033[0;31mHome Directory : \033[0m"<<home<<endl;
	if(!mode)
	{
		setCursor(18,0);
		cout<<"-----------------------------------------------------------------------------------------------------"<<endl;
		cout<<"\033[1;32m"<<"Command : ~$ "<<"\033[0m";
	}
	return;
}
void get_files(char const *path)
{
	files.clear();
	struct dirent *entry;
        dir = opendir(path);
        if (dir == NULL) 
        {
          return;
        }
        chdir(path);
        getcwd(cwd,1024);        
        while ((entry = readdir(dir)) != NULL) 
        {
        	files.push_back(entry);
	}
        closedir(dir);
        s = 0 ;
        e = (files.size()>MAX)? MAX : files.size();
        cursor = 0;
        printFiles();
        return;
}


/*Key Press Functions of Normal Mode starts */
void pressUp()
{
	if(cursor==1 && s==0)
		return;
	if(cursor>1)
	{
		cursor--;
		setCursor(cursor,0);
		return;
	}
	else if(s==0)
	{	
		return;
	}
	s-=1;
	e-=1;
	printFiles();
	setCursor(cursor,0);
	return;
}
void pressDown()
{
	if(cursor<files.size() && cursor<MAX)
	{
		cursor++;
		setCursor(cursor,0);
		return;
	}
	if(e==files.size())
	{
		return;
	}
	s+=1;
	e+=1;
	printFiles();
	setCursor(cursor,0);
	return;	
}
void pressK()
{
	s = max(s-MAX, 0);
    	e = s+MAX;
        printFiles();
    	setCursor(cursor,0);
   	return;
}
void pressL()
{
	e = (e+MAX)>files.size() ? files.size() : (e+MAX);
	s = e-MAX;
	printFiles();
	setCursor(cursor,0);
	return;
}
void pressBackspace()
{
	if(cwd!=home)
	{
		bstack.push(cwd);
		get_files("../");
	}
	setCursor(1,1);
	return;
}
void pressEnter()
{
	struct stat info;
	char *fname = files[cursor+s-1]->d_name;
	lstat(fname,&info);
	
	if(S_ISDIR(info.st_mode))
	{
        	if(strcmp(fname,"..")==0 )
        	{
            		pressBackspace();
            		setCursor(1,1);
            		return;
        	}  
    	    	if(strcmp(fname,".")==0)
    	    	{
    	   	 	setCursor(1,1);
    	   	 	return;
    	   	}
    		
    		bstack.push(string(cwd));
    		string str = string(cwd) +"/"+fname;
             	get_files(str.c_str());
    	}
    	else
    	{
        	pid_t pid=fork();
       	if(pid==0)
       	{
       		execl("/usr/bin/xdg-open","xdg-open",fname,NULL);
            		exit(1);
        	}
    	}
    	setCursor(1,1);
    	return;
}
void pressH()
{
	if(cwd != home)
	{
		bstack.push(string(cwd));
		get_files(home);
	}
	setCursor(1,1);
	return;
}

void pressLeft()
{
	if(bstack.empty())
		return;
	string pd = bstack.top();
	bstack.pop();
	fstack.push(string(cwd));
	get_files(pd.c_str());
	setCursor(1,1);
	return;
}

void pressRight()
{
	if(fstack.empty())
		return;
		
	string pd = fstack.top();
	fstack.pop();
	bstack.push(string(cwd));
	get_files(pd.c_str());
	setCursor(1,1);
	return;
}
/*Normal Mode Function Ends */


/*Command Mode Function Starts*/
/*********************Delete Function Starts ***********************************/
void delete_file()
{
	string temp;
	string filename;
	for(int i=1;i<command.size();i++)
		temp = temp + command[i]+" ";
	temp.erase(temp.end()-1);
	if(temp[0] ==126)
	{
		temp.erase(temp.begin());
		filename = string(home) +  temp;
	}
	else if(temp[0] == 47)
	{
		filename = temp;
	}
	else
		filename = string(home) + "/" + temp;
	int s = unlink(filename.c_str());
	if(s==-1)
		msg = "File not deleted";
	else
		msg = "File Deleted successfully";
	return;
}
void delete_dir_rec(string des)
{
	DIR *d;
    	struct dirent *dr;
    	d = opendir(des.c_str());
    	struct stat info;
    	if (d == NULL)
        	return;
	chdir(des.c_str());
    	while ((dr = readdir(d)) != NULL)
    	{
    		lstat(dr->d_name,&info);
        	if (S_ISDIR(info.st_mode))
        	{
        		string cdes = des + "/" + dr->d_name;
        		if (!strcmp(dr->d_name, ".") || !strcmp(dr->d_name, ".."))
        			continue;
        		
               	delete_dir_rec(cdes);
                	rmdir(cdes.c_str());
                }
            	
            	else
            	{
            		unlink(dr->d_name);
            	}
    	}
    	chdir("..");
    	closedir(d);
    	return;
	
}
void delete_dir()
{
	string temp;
	string filename;
	for(int i=1;i<command.size();i++)
		temp = temp + command[i]+" ";
	temp.erase(temp.end()-1);
	if(temp[0] ==126)
	{
		temp.erase(temp.begin());
		filename = string(home) +  temp;
	}
	else if(temp[0] == 47)
	{
		filename = temp;
	}
	else
		filename = string(home) + "/" + temp;
	
	delete_dir_rec(filename);
	rmdir(filename.c_str());
	if(s==-1)
		msg = "Directory  Deleted Successfully";
	else
		msg = "Error in Deleting Directory";
	return;
}
/*********************Delete Function Ends ***********************************/
/*********************Copy Function Starts ***********************************/
void copy_file(string src, string des)
{
	char cp[1024];
	int fin, fout,nread;
    	fin = open((src).c_str(),O_RDONLY);
	fout = open((des).c_str(),O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
	while((nread = read(fin,cp,sizeof(cp)))>0)
		write(fout,cp,nread);
	if(fout)
		msg = "File Copied successfully";
	else
		msg = "Error in Copying file";
	if(mv==true)
	{
		msg = "File Moved successfully";
		unlink(src.c_str());
	}
	return;
}
void copy_dir(string src, string des)
{	
	DIR *d;
    	struct dirent *dr;
    	d = opendir(src.c_str());
    	struct stat info;
    	mkdir(des.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    	if (d == NULL)
    	{
    		if(mv)
    			msg = "Error in Moving Directory";
    		else
    			msg = "Error in Copying Directory";
        	return;
        }
    	while ((dr = readdir(d)) != NULL)
    	{
        	if (!strcmp(dr->d_name, ".") || !strcmp(dr->d_name, ".."));
        	else
        	{
        		lstat(src.c_str(),&info);
        		string csrc = src + "/" + dr->d_name;
           		string cdes = des + "/" + dr->d_name;
           		if (S_ISDIR(info.st_mode))
                		copy_dir(csrc, cdes);
            		else
            			copy_file(csrc, cdes);
            	
        	}
    	}
    	msg = "Directory Copied Successfully";
    	if(mv)
    	{
    		msg = "Directory Moved Successfully";
    		delete_dir_rec(src);
		rmdir(src.c_str());
	}
	return;
}
void copy()
{
	string des;
	int f=0;
	int nf =0;
	for(int i=1;i<command.size();i++)
	{
		if(command[i][0] == 126)
			f = 1;
		if(command[i][0] == 46)
		{
			des = home;
			break;
		}
		if(command[i][0] == 47)
		{
			des = " ";
			f=1;
		}
		if(f!=0)
			des = des + command[i]+ " ";
		else
			nf++;
	}
	if(des[0] == 126)
	{
		des.erase(des.begin());
		des.erase(des.end()-1);
		des = home + des;
	}
	else if(des[0]== 32)
	{
		des.erase(des.begin());
		des.erase(des.end()-1);
	}
	struct stat info;
	for(int i=1;i<=nf;i++)
	{
		string src = string(home) + "/" + command[i];
		string fdes = des + "/" +command[i];
		lstat(src.c_str(), &info);
		
		if(S_ISDIR(info.st_mode))
			copy_dir(src, fdes);
		else
			copy_file(src, fdes);
	}
}
/*********************Copy Function Ends ***********************************/
/*********************Rename Function Starts ***********************************/
void Rename()
{
	if(rename(command[1].c_str(),command[2].c_str())==0)
		msg = "File renamed successfully";
	else
		msg = "Error in renaming File/Directory";
	return;
}
/*********************Rename Function Ends ***********************************/
/*********************Create File/Directory Function Starts ***********************************/
void create_file()
{
	string temp;
	string filename;
	for(int i=2;i<command.size();i++)
		temp = temp + command[i]+" ";
	temp.erase(temp.end()-1);
	ofstream file;
	if(temp[0] ==126)
	{
		temp.erase(temp.begin());
		filename = string(home) +  temp;
	}
	else
	{
		filename = string(home);
	}
	filename = filename + "/" + command[1];
	file.open(filename,ios::out);
	if(file)
		msg = "File created successfully";
	else
		msg = "Error in creating new file";
	setCursor(24,0);
	file.close();
	return;
}
void create_dir()
{
	string temp;
	string filename;
	for(int i=2;i<command.size();i++)
		temp = temp + command[i]+" ";
	temp.erase(temp.end()-1);
	if(temp[0] ==126)
	{
		temp.erase(temp.begin());
		filename = string(home) +  temp;
	}
	else
	{
		filename = string(home);
	}
	filename = filename + "/" + command[1];
	if(mkdir((filename).c_str(),S_IRUSR|S_IWUSR|S_IXUSR)==0)
		msg = "Directory created successfully";
	else
		msg = "Error in creating new Directory";
	return;
}
/*********************Create File/Directory Function Ends ***********************************/


/*********************goto Function Starts ***********************************/
void Goto()
{
	string str="";
	for(int i=1;i<command.size();i++)
	{
		str= str + command[i] + " ";
	}
	str.erase(str.end()-1);
	if(str=="/")
		get_files(home);
	else
		get_files(str.c_str());
}
/*********************goto Function Ends ***********************************/
/*********************Search Function Starts ***********************************/
bool search_rec(string des, string s)
{
	DIR *d;
	struct dirent *dr;
	struct stat info;

	d = opendir(des.c_str());
	if (d == NULL)
        	return false;
	chdir(des.c_str());
	while((dr = readdir(d))){
		lstat(dr->d_name,&info);
		string name =  string(dr->d_name);
		if(s == name)
		{	
			get_files(des.c_str());
			return true;
		}
		if(S_ISDIR(info.st_mode)){
			if( (name == ".") || (name == "..") )
			{
				continue;
			}
			bool t =  search_rec(des + '/' + name, s);
			if(t) 
				return true;
		}
	}
	chdir("..");
	closedir(d);
	return false;
}
bool search()
{
	return search_rec(cwd,command[1]);
}

/*********************Search Function Ends ***********************************/
/*********************Move Function Starts ***********************************/
void move_file()
{
	mv = true;
	copy();
	mv = false;
	return;
}
void move_dir()
{
	mv= true;
	copy();
	mv = false;
	return;
}
void move()
{
	move_file();
}
/*********************Move Function Ends ***********************************/
void processInput()
{
	command.clear();
	char ch;
	string ip;
	while(true)
	{
		ch = cin.get();
		if(ch==10)
			break;
		if(ch==27)
		{
			mode = true;
			printFiles();
			break;
		}
		if(ch==127)
		{
			if(ip.size()>0)
				ip.pop_back();
			printFiles();
			cout<<ip;
		}
		else
		{
			ip.push_back(ch);
			cout<<ch;
		}
	}
	
	string temp ="";
	for(int i=0;i<=ip.size();i++)
	{
		
		if(i==ip.length() || ip[i]==' ')
		{
			command.push_back(temp);
			temp = "";
		}
		else
			temp+= ip[i];
	}
	string cmd = command[0];
	if(cmd=="copy")
		copy();
	if(cmd=="move")
		move();
	if(cmd=="rename")
		Rename();
	if(cmd=="create_file")
		create_file();	
	if(cmd=="create_dir")
		create_dir();
	if(cmd=="delete_file")
		delete_file();
	if(cmd=="delete_dir")
		delete_dir();
	if(cmd=="goto")
		Goto();
	if(cmd=="search")
	{
		setCursor(23,0);
		if(search())
			msg ="File Found Successfully";
		else
			msg = "File not found";
	}
	return;
}
void commandMode()
{
	mode = false;
	printFiles();
	setCursor(19,0);
	while(true)
	{
		command.clear();
		setCursor(19,14);
		msg = "";
		processInput();
		get_files(cwd);
		printFiles();
		setCursor(22,0);
		cout<<"\033[0;36m"<<msg<<"\033[0m"<<endl;
		setCursor(19,14);
		msg = "";
		if(mode)
			break;
	}
	return;
}

/*Command Mode Fnctions end*/
void normal_mode(char *cwd)
{
	cout<<"\033[H\033[J";
	enableRawMode();
	strcpy(home,cwd);
	ioctl(STDOUT_FILENO,TIOCGWINSZ,&w);
	get_files(cwd);
	setCursor(1,1);	
	char ch=' ';
    	while(ch!='q'){
            ch=cin.get();               
            switch(ch)
            { 
            	case 58: commandMode();
            		mode = true;
            		get_files(cwd);
			printFiles();
			setCursor(0,0);
            		break;
            	case 10: pressEnter();
            		break;
            	case 65: pressUp();
            		break;
            	case 66: pressDown();
            		break;
            	case 67: pressRight();
            		break;
            	case 68: pressLeft();
            		break;
            	case 104: pressH();
            		break;
            	case 107 : pressK();
            		break;
            	case 108 : pressL();
            		break;
            	case 127: pressBackspace();
            		break;
            	default : break;
            }
        }
        cout<<"\033[H\033[J";
	atexit(disableRawMode);
}

int main()
{
	cout<<"\033[H\033[J";
	getcwd(cwd, 1024);
	normal_mode(cwd);
	return 0;
}
