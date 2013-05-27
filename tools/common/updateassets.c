/*
	Onion HTTP server library
	Copyright (C) 2012 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#include <malloc.h>
#include <onion/log.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "updateassets.h"


struct onion_assets_file_t{
	FILE *file;
	char **lines;
	int lines_count;
	int lines_capacity;
	char has_endif;
};

void onion_assets_file_add_line(onion_assets_file *f, const char *line){
	if (strcmp(line,"#endif")==0){
		f->has_endif=1;
		return;
	}
	
	if (f->lines_count==f->lines_capacity){
		if (f->lines_capacity < 2048) 
			f->lines_capacity*=2;
		else
			f->lines_capacity+=2048;
		f->lines=realloc(f->lines, sizeof(const char *)*f->lines_capacity);
	}
	f->lines[f->lines_count++]=strdup(line);
	ONION_DEBUG("Add line: %s", line);
}

onion_assets_file *onion_assets_file_new(const char *filename){
	onion_assets_file *ret=malloc(sizeof(onion_assets_file));
	ret->file=fopen(filename, "rt");
	ret->lines_count=0;
	ret->lines_capacity=16;
	ret->lines=malloc(sizeof(const char *)*ret->lines_capacity);
	ret->has_endif=0;
	if (!ret->file){
		ret->file=fopen(filename,"wt");
		if (!ret->file){
			ONION_WARNING("Could not open %s asset file for updating.", filename);
			return NULL;
		}
		onion_assets_file_add_line(ret,"/* Autogenerated by onion assets */");
		onion_assets_file_add_line(ret,"#ifndef __ONION_ASSETS_H__");
		onion_assets_file_add_line(ret,"#define __ONION_ASSETS_H__");
		onion_assets_file_add_line(ret,"#endif");
	}
	else{
		char buffer[4096];
		int r;
		int o=0;
		do{
			r=fread(&buffer[o],1, sizeof(buffer)-o, ret->file);
			int i;
			o=0;
			for (i=0;i<r;i++){
				if (buffer[i]=='\n'){
					buffer[i]=0;
					onion_assets_file_add_line(ret, &buffer[o]);
					o=i+1;
				}
			}
			assert(o!=0 && r>0); // "Line is longer than buffer size or EOF without EOL.");
		}while(r<0);
		fclose(ret->file);
		ret->file=fopen(filename,"wt");
	}
	return ret;
}

int onion_assets_file_update(onion_assets_file* file, const char* line){
	int i;
	for (i=0;i<file->lines_count;i++){
		if (strcmp(file->lines[i],line)==0)
			return 0;
	}
	onion_assets_file_add_line(file, line);
	return 1;
}


int onion_assets_file_free(onion_assets_file *f){
	fseek(f->file, 0, SEEK_SET);
	
	int i;
	for (i=0;i<f->lines_count;i++){
		ONION_DEBUG("Write: %s", f->lines[i]);
		ssize_t length=strlen(f->lines[i]);
		ssize_t wlength=fwrite(f->lines[i], 1, length, f->file);
		if (wlength!=length){
			ONION_ERROR("Could not write all data. Aborting");
			abort();
		}
		wlength=fwrite("\n",1, 1, f->file);
		if (wlength!=1){
			ONION_ERROR("Could not write all data. Aborting");
			abort();
		}
		free(f->lines[i]);
	}
	free(f->lines);
	
	if (f->has_endif)
		fprintf(f->file, "#endif\n");
	
	assert(fclose(f->file)==0);
	free(f);
	return 0;
}

