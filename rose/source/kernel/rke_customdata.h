#pragma once

#include "lib/lib_utildefines.h"
#include "lib/lib_error.h"
#include "lib/lib_alloc.h"

#include "dna/dna_customdata_types.h"
#include "dna/dna_meshdata_types.h"
#include "dna/dna_vec_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	struct AnonymousAttributeID;
	struct CustomData;

	typedef enum eCDAllocType {
		CD_ASSIGN = 0 , // Use the data pointer.
		CD_SET_DEFAULT = 2 , // Allocate and set to default, which is usually just zeroed memory.
		CD_REFERENCE = 3 , // Use data pointers, set layer flag NOFREE.
		CD_DUPLICATE = 4 , // Do a full copy of all layers, only allowed if source has same number of elements.
		CD_CONSTRUCT = 5 , // Default construct new layer values. Does nothing for trivial types.
	} eCDAllocType;

	/**
	* Checks if the layer at physical offset \a layer_n (in data->Layers) support math
	* the below operations.
	*/
	bool CustomData_layer_has_math ( const struct CustomData *data , int layer_n );

	/**
	* Checks if the layer at physical offset \a layer_n (in data->Layers) support interpolation
	* the below operations.
	*/
	bool CustomData_layer_has_interp ( const struct CustomData *data , int layer_n );

	/**
	* Checks if any of the custom-data layers has math.
	*/
	bool CustomData_has_math ( const struct CustomData *data );

	/**
	* Checks if any of the custom-data layers has interpolation.
	*/
	bool CustomData_has_interp ( const struct CustomData *data );

	/**
	* Compares if data1 is equal to data2.
	* \param type Any of the valid CustomData type enum (e.g. #CD_MLOOPUV).
	* \note The layer type's equal function is used to compare 
	* the data, if it exists, otherwise #memcmp is used.
	* \return Returns \c True if the data are equal.
	*/
	bool CustomData_data_equals ( int type , const void *data1 , const void *data2 );

	void CustomData_data_multiply ( int type , void *data , float factor );
	void CustomData_data_add ( int type , void *data1 , const void *data2 );

	/** 
	* Recalculate the index of each layer in the customdata's TypeMap so that the 
	* indices match the layers.
	*/
	void CustomData_update_typemap ( CustomData *data );

	/** 
	* Resize the data of all the layers that do not have the CD_FLAG_NOFREE flag 
	* to \a totelem elements.
	*/
	void CustomData_realloc ( CustomData *data , const int totelem );

	/**
	* NULL's all members and resets the #CustomData.typemap.
	*/
	void CustomData_reset ( struct CustomData *data );

	/**
	* Frees data associated with a CustomData object (doesn't free the object itself, though).
	*/
	void CustomData_free ( struct CustomData *data , int totelem );

	/**
	* Frees all layers with #CD_FLAG_TEMPORARY.
	*/
	void CustomData_free_temporary ( struct CustomData *data , int totelem );

	/**
	* Returns the index of the first layer that matches the specified type in the customdata layer array.
	* (offset from the start of the layer's array).
	*/
	int CustomData_get_layer_index ( const CustomData *data , const int type );

	/**
	* Returns the index of the n-th layer that matches the specified type in the customdata layer array.
	* (offset from the start of the layer's array).
	*/
	int CustomData_get_layer_index_n ( const CustomData *data , const int type , const int n );

	/**
	* Returns the index of the layer with the matching name of the specified type in the customdata layer array.
	* (offset from the start of the layer's array).
	*/
	int CustomData_get_named_layer_index ( const CustomData *data , const int type , const char *name );

	/** 
	* Returns the index of the active layer with the specified type in the customdata layer array.
	* (offset from the start of the layer's array).
	*/
	int CustomData_get_active_layer_index ( const struct CustomData *data , const int type );

	/** 
	* Update the name of the n-th layer of the speicifed type in the customdata.
	*/
	bool CustomData_set_layer_name ( struct CustomData *data , int type , int n , const char *name );

	/**
	* Returns the active layer of the specified type, the index of the layer is 
	* (offset from the first layer with the specified type).
	*/
	int CustomData_get_named_layer ( const CustomData *data , const int type , const char *name );

	/**
	* Returns name of the active layer of the given type or NULL if no such active layer is defined.
	*/
	const char *CustomData_get_active_layer_name ( const struct CustomData *data , int type );

	/**
	* Gets a pointer to the data element at index from the first (active) layer of type.
	* \return NULL if there is no layer of type.
	*/
	void *CustomData_get ( const struct CustomData *data , int index , int type );

	/**
	* Gets a pointer to the data element at index from the n-th layer of type.
	* \return NULL if there is no layer of type.
	*/
	void *CustomData_get_n ( const struct CustomData *data , int type , int index , int n );

	/**
	* Copies the data from source to the data element at index in the first (active) layer of type 
	* no effect if there is no layer of type.
	*/
	void CustomData_set ( const struct CustomData *data , int index , int type , const void *source );

	/**
	* Set the pointer of to the first (active) layer of type. the old data is not freed.
	* returns the value of `ptr` if the layer is found, NULL otherwise.
	*/
	void *CustomData_set_layer ( const struct CustomData *data , int type , void *ptr );

	/**
	* Set the pointer of to the n-th layer of type. the old data is not freed.
	* returns the value of `ptr` if the layer is found, NULL otherwise.
	*/
	void *CustomData_set_layer_n ( const struct CustomData *data , int type , int n , void *ptr );


	/**
	* Adds a data layer of the given type to the #CustomData object, optionally 
	* backed by an external data array. the different allocation types are 
	* defined above. returns the data of the layer.
	*/
	void *CustomData_add_layer ( struct CustomData *data , int type , eCDAllocType alloctype , void *layer , int totelem );

	/**
	* Same as #CustomData_add_layer but also accepts name. 
	* "Adds a data layer of the given type to the #CustomData object, optionally 
	* backed by an external data array. the different allocation types are 
	* defined above. returns the data of the layer." 
	*/
	void *CustomData_add_layer_named ( struct CustomData *data ,
					   int type ,
					   eCDAllocType alloctype ,
					   void *layer ,
					   int totelem ,
					   const char *name );

	/**
	* Same as #CustomData_add_layer but name is taken from a specified AnonymousAttributeID. 
	* "Adds a data layer of the given type to the #CustomData object, optionally
	* backed by an external data array. the different allocation types are
	* defined above. returns the data of the layer."
	*/
	void *CustomData_add_layer_anonymous ( struct CustomData *data ,
					       int type ,
					       eCDAllocType alloctype ,
					       void *layer ,
					       int totelem ,
					       const struct AnonymousAttributeID *anonymous_id );

#ifdef __cplusplus
}
#endif
