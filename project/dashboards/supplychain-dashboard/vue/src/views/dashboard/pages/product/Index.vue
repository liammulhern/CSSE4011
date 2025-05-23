<script setup lang="ts">
import { onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useProductStore } from '@/stores/product'
import { Card, CardHeader, CardTitle, CardContent, CardDescription } from '@/components/ui/card'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { Button } from '@/components/ui/button'

const route = useRoute();
const productStore = useProductStore();
const productId = route.params.id as string;

onMounted(() => {
  productStore.fetchProduct(productId);
})
</script>

<template>
  <div>
    <page-header title="Product">
      <div class="flex items-center space-x-2">
        <Button variant="secondary">Edit</Button>
        <Button>Delete</Button>
      </div>
    </page-header>

    <Tabs default-value="overview" class="space-y-4">
      <TabsList>
        <TabsTrigger value="overview">Overview</TabsTrigger>
        <TabsTrigger value="analytics">Analytics</TabsTrigger>
        <TabsTrigger value="history">History</TabsTrigger>
      </TabsList>

      <!-- Overview Tab -->
      <TabsContent value="overview" class="space-y-4">
        <div class="grid gap-4 md:grid-cols-2 lg:grid-cols-4">
          <Card>
            <CardHeader class="flex items-center justify-between pb-2">
              <CardTitle class="text-sm font-medium">Price</CardTitle>
            </CardHeader>
            <CardContent>
              <div class="text-2xl font-bold">
                <!-- ${{ productStore.product.price.toFixed(2) }} -->
              </div>
            </CardContent>
          </Card>

          <Card>
            <CardHeader class="flex items-center justify-between pb-2">
              <CardTitle class="text-sm font-medium">Stock</CardTitle>
            </CardHeader>
            <CardContent>
              <div class="text-2xl font-bold">
                <!-- {{ productStore.product.stock }} -->
              </div>
            </CardContent>
          </Card>

          <Card>
            <CardHeader class="flex items-center justify-between pb-2">
              <CardTitle class="text-sm font-medium">SKU</CardTitle>
            </CardHeader>
            <CardContent>
              <div class="text-2xl font-bold">
                <!-- {{ productStore.product.sku }} -->
              </div>
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle class="text-sm font-medium">Category</CardTitle>
            </CardHeader>
            <CardContent>
              <div class="text-2xl font-bold">
                <!-- {{ productStore.product.category }} -->
              </div>
            </CardContent>
          </Card>
        </div>

        <Card>
          <CardHeader>
            <CardTitle>Description</CardTitle>
            <CardDescription>
              Brief overview
            </CardDescription>
          </CardHeader>
          <CardContent>
            <!-- {{ productStore.product.description }} -->
          </CardContent>
        </Card>
      </TabsContent>

      <!-- Analytics Tab -->
      <TabsContent value="analytics" class="space-y-4">
        <div class="grid gap-4 md:grid-cols-2">
          <Card class="col-span-2">
            <CardHeader>
              <CardTitle>Sales Overview</CardTitle>
            </CardHeader>
            <CardContent class="pl-2">
              <!-- <SalesOverview :data="productStore.product.salesData" /> -->
            </CardContent>
          </Card>
          <Card class="col-span-2">
            <CardHeader>
              <CardTitle>Recent Orders</CardTitle>
            </CardHeader>
            <CardContent>
              <!-- <RecentOrders :orders="productStore.product.recentOrders" /> -->
            </CardContent>
          </Card>
        </div>
      </TabsContent>

      <!-- History Tab -->
      <TabsContent value="history" class="space-y-4">
        <Card>
          <CardHeader>
            <CardTitle>Change Log</CardTitle>
          </CardHeader>
          <CardContent>
            <ul class="list-disc pl-5 space-y-2">
              <!-- <li v-for="event in productStore.product.history" :key="event.id"> -->
              <!--   <span class="font-medium">{{ event.date }}:</span> {{ event.change }} -->
              <!-- </li> -->
            </ul>
          </CardContent>
        </Card>
      </TabsContent>
    </Tabs>
  </div>
</template>
