<script setup lang="ts">
import { ref } from 'vue'
import ProductStatusTable from '@/views/dashboard/pages/home/ProductStatusTable.vue'
import NewProductForm from '@/views/dashboard/pages/products/product/NewProductForm.vue'

// Control showing the form
const showForm = ref(false)

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
