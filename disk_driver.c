#include <fcntl.h>   
#include <sys/stat.h> 
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

#include "disk_driver.h"

void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){
	if(disk == NULL || filename == NULL || num_blocks < 1){
		printf("ERRORE: parametri non validi\n");
		return;
	}
	
	int bitmap_size = num_blocks / 8;	// Dimensione bitmap, ogni blocco nel disco lo rappresento con 1 bit della bitmap
	if(num_blocks % 8) ++bitmap_size;	// Arrotondo
	
	int fd;	// File descriptor da restituire
	// Controllo se posso accedere al filename e accedo / creo
	int can_access = access(filename, F_OK) == 0;
	
	if(can_access){
		fd = open(filename, O_RDWR, (mode_t)0666);
		if(!fd){
			printf("Errore nell'apertura del file\n");
			return;
		}
	}
	else{
		fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, (mode_t)0666);
		if(!fd){
			printf("Errore nella creazione del file\n");
			return;
		}
		
		// Alloco il nuovo disco
		int ret;
		ret = posix_fallocate(fd, 0, sizeof(DiskHeader) + bitmap_size);
		if(ret){
			printf("Errore nell'allocazione del disco");
			return;
		}
	}

	// Devo mappare il disco in memoria da restituire
	DiskHeader* disco_mappato = (DiskHeader*) mmap(0,
													sizeof(DiskHeader) + bitmap_size,
													PROT_READ | PROT_WRITE,
													MAP_SHARED,
													fd,
													0); 
	if(disco_mappato == MAP_FAILED){
		close(fd);
		printf("ERROR: mmap()\n");
		return;
	}
	
	// Lo setto 
	if(!can_access){
		disco_mappato->num_blocks = num_blocks;
		disco_mappato->bitmap_blocks = num_blocks;
		disco_mappato->bitmap_entries = bitmap_size;
		
		disco_mappato->free_blocks = num_blocks;
		disco_mappato->first_free_block = 0;
	}

	// Lo salvo
	disk->header = disco_mappato;
	disk->bitmap_data = (char*)disco_mappato + sizeof(DiskHeader);
	disk->fd = fd;
}


int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
	if(block_num > disk->header->bitmap_blocks || block_num < 0 || dest == NULL || disk == NULL){
		printf("ERRORE: parametri non validi\n");
		return -1;
	}

	// Uso la bitmap per controllare se il blocco ?? libero
	BitMap bmp;
	bmp.num_bits = disk->header->bitmap_blocks;
	bmp.entries = disk->bitmap_data;

	// Se il blocco ?? vuoto non lo leggo
	if(!BitMap_getBit(&bmp, block_num)) return -1;
	
	// Lo leggo
	int ret = pread(disk->fd,
			dest,
			BLOCK_SIZE,
			sizeof(DiskHeader) + disk->header->bitmap_entries + (block_num*BLOCK_SIZE));
	if(ret < 0){
		printf("ERRORE: pread()\n");
		return -1;
	}

	return 0;
}


int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
	if(block_num > disk->header->bitmap_blocks || block_num < 0 || src == NULL || disk == NULL){
		printf("ERRORE: parametri non validi\n");
		return -1;
	}

	BitMap bmp;
	bmp.num_bits = disk->header->bitmap_blocks;
	bmp.entries = disk->bitmap_data;

	// Se il blocco ha status 1 nella bitmap non scrivo
	if(BitMap_getBit(&bmp, block_num)) return -1;
	
	// Se sto scrivendo sull'attuale primo blocco libero, aggiorno il primo blocco libero
	// partendo dal successivo
	if(block_num == disk->header->first_free_block)
		disk->header->first_free_block = DiskDriver_getFreeBlock(disk, block_num + 1);
	
	// Setto che il blocco ?? occupato ora
	if(BitMap_set(&bmp, block_num, 1) == -1) return -1;

	// Aggiorno il n blocchi liberi
	disk->header->free_blocks -= 1;
	
	// Lo scrivo
	int ret = pwrite(disk->fd,
			src,
			BLOCK_SIZE,
			sizeof(DiskHeader) + disk->header->bitmap_entries + (block_num*BLOCK_SIZE));
	if(ret < 0){
		printf("ERRORE: pwrite()\n");
		return -1;
	}

	return 0;
}

int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
	if(start > disk->header->bitmap_blocks){
		printf("ERRORE: parametri non validi\n");
		return -1;
	}
	
	BitMap bmp;
	bmp.num_bits = disk->header->bitmap_blocks;
	bmp.entries = disk->bitmap_data;
	
	// Prendo e restitusco il primo blocco libero 
	//guardando la bitmap con primo status a 0
	return BitMap_get(&bmp, start, 0);
}

int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
	if(block_num > disk->header->bitmap_blocks || block_num < 0 || disk == NULL){
		printf("ERRORE: parametri non validi\n");
		return -1;
	}
	
	BitMap bmp;
	bmp.num_bits = disk->header->bitmap_blocks;
	bmp.entries = disk->bitmap_data;
	
	// Verifico che il bit non sia gi?? libero
	if(!BitMap_getBit(&bmp, block_num)) return -1;
	
	// Allora lo setto a 0 se non ?? libero
	if(BitMap_set(&bmp, block_num, 0) < 0){
		printf("ERRORE: BitMap_set() in freeBlock\n");
		return -1;
	}
	
	// Devo aggiornare il primo blocco libero
	// se il numero del blocco ?? minore del primo libero
	if(block_num < disk->header->first_free_block)
		disk->header->first_free_block = block_num;
	
	// Aggiorno il numero dei blocchi liberi
	disk->header->free_blocks++;
	
	return 0;
}

// msync: scarica le modifiche apportate alla copia interna di un file che
// ?? stato mappato in memoria
int DiskDriver_flush(DiskDriver* disk){
	int bitmap_size = disk->header->num_blocks / 8;	// Dimensione bitmap
	if(disk->header->num_blocks % 8) bitmap_size++;	// Arrotondo

	int ret = msync(disk->header,
			(size_t)sizeof(DiskHeader) + bitmap_size,
			MS_SYNC);
	if(ret < 0){
		printf("ERRORE: msync() in flush");
		return -1;
	}

	return 0;
}

// Come la write, ma devo solo aggiornare un blocco quindi non mi serve la bitmap
// la write non scrive su un blocco gi?? occupato
int DiskDriver_updateBlock(DiskDriver* disk, void* src, int block_num){
	if(block_num > disk->header->bitmap_blocks || block_num < 0 || src == NULL || disk == NULL){
		printf("ERRORE: parametri non validi\n");
		return -1;
	}

	int fd = disk->fd;
		
	int ret = pwrite(fd, src, BLOCK_SIZE, sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE));
	if(ret < 0){
		printf("ERRORE: pwrite() in updateBlock");
		return -1;
	}
	return 0;
}