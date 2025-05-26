<script setup lang="ts">
import OrderStatusTable from '@/views/dashboard/pages/home/OrderStatusTable.vue'
import NewProductOrderForm from '@/views/dashboard/pages/product_orders/order/NewProductOrderForm.vue'
</script>

<template>
  <div>
    <page-header title="Product Orders">
      <NewProductOrderForm />
    </page-header>
    <OrderStatusTable tableName="Product Orders" />
  </div>
</template>
