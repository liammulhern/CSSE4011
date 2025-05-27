<script setup lang="ts">
import ProductTypeStatusTable from '@/views/dashboard/pages/home/ProductTypeStatusTable.vue'
import NewProductTypeForm from '@/views/dashboard/pages/product_types/type/NewProductTypeForm.vue'

</script>

<template>
    <div>
        <page-header title="Product Catalog">
            <NewProductTypeForm />
        </page-header>
        <div>
            <ProductTypeStatusTable tableName="Types" />
        </div>
    </div>
</template>
