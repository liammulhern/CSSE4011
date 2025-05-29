<script setup lang="ts">
import ProductStatusTable from '@/views/dashboard/pages/home/ProductStatusTable.vue'
import NewProductForm from '@/views/dashboard/pages/products/product/NewProductForm.vue'

</script>

<template>
    <div>
        <page-header title="Products">
            <NewProductForm />
        </page-header>
        <div>
            <ProductStatusTable tableName="Products" />
        </div>
    </div>
</template>
