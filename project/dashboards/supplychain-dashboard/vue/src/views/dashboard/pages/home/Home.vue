<script setup lang="ts">
import { onMounted, computed, watch, ref } from 'vue'
import { useAuthStore } from '@/stores/auth'

// Tabs & UI
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/card'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { DateRangePicker } from '@/components/ui/daterange-picker'

// Dashboard pieces
import OrderStatusTable from '@/views/dashboard/pages/home/OrderStatusTable.vue'
import LiveEventFeed from '@/views/dashboard/pages/home/LiveEventFeed.vue'
import ComplianceAlerts from '@/views/dashboard/pages/home/ComplianceAlerts.vue'
import RecentNotifications from '@/views/dashboard/pages/home/RecentNotifications.vue'
import ProductEventMap from '@/components/maps/ProductEventMap.vue'

// Pull in our ProductOrder store
import { useProductOrderStore } from '@/stores/productorder'

const auth = useAuthStore()
const orderStore = useProductOrderStore()

// Derived/computed bindings for template
const totalOrders = computed(() => orderStore.totalOrders)
const totalOrdersDelta = computed(() => orderStore.totalOrdersDelta)
const inTransitOrders = computed(() => orderStore.inTransitOrders)
const inTransitOrdersDelta = computed(() => orderStore.inTransitOrdersDelta)
const deliveredOrders = computed(() => orderStore.deliveredOrders)
const deliveredOrdersDelta = computed(() => orderStore.deliveredOrdersDelta)
const complianceAlerts = computed(() => orderStore.complianceAlerts)
const complianceAlertsDelta = computed(() => orderStore.complianceAlertsDelta)

// Date range state for the picker
const dateRange = ref<{ start: Date | null; end: Date | null }>({
  start: null,
  end: null,
})

const productOrderStore = useProductOrderStore();

onMounted(async () => {
  if (!auth.user) {
    await auth.fetchUser()
  }
  // fetch summary for the last week by default
  const now = new Date()
  const yesterday = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000)
  dateRange.value = { start: yesterday, end: now }
  orderStore.fetchSummary(yesterday, now.toISOString())
})

watch(
  dateRange,
  ({ start, end }) => {
    if (start && end) {
      const s = (start instanceof Date ? start : new Date(start)).toISOString();
      const e = (end instanceof Date ? end : new Date(end)).toISOString();
      productOrderStore.fetchSummary(s, e);
      productOrderStore.fetchOrders(1000, s, e);
    }
  },
  { immediate: true, deep: true }
)
</script>

<template>
  <div>
    <page-header title="Dashboard">
      <div class="flex items-center space-x-2">
        <DateRangePicker v-model:range="dateRange" />
      </div>
    </page-header>

    <Tabs default-value="overview" class="space-y-4">
      <TabsList>
        <TabsTrigger value="overview">Overview</TabsTrigger>
        <TabsTrigger value="events">Events</TabsTrigger>
        <TabsTrigger value="analytics">Analytics</TabsTrigger>
        <TabsTrigger value="map">Map</TabsTrigger>
      </TabsList>

      <!-- OVERVIEW: summary + quick orders -->
      <TabsContent value="overview" class="space-y-4">
        <div class="grid gap-4 md:grid-cols-2 lg:grid-cols-4">
          <Card>
            <CardHeader class="flex flex-row items-center justify-between space-y-0 pb-2">
              <CardTitle class="text-sm font-medium">
                Total Product Orders
              </CardTitle>
              <Icon name="Package" class="h-4 w-4 text-muted-foreground" />
            </CardHeader>
            <CardContent>
              <div class="text-2xl font-bold">{{ totalOrders }}</div>
              <p class="text-xs text-muted-foreground">
                {{ totalOrdersDelta >= 0 ? '+' : '' }} {{ totalOrdersDelta }} since {{ dateRange.start ?
                  dateRange.start.toLocaleDateString() : 'start' }}
              </p>
            </CardContent>
          </Card>

          <Card>
            <CardHeader class="flex flex-row items-center justify-between space-y-0 pb-2">
              <CardTitle class="text-sm font-medium">
                Orders In-Transit
              </CardTitle>
              <Icon name="Truck" class="h-4 w-4 text-muted-foreground" />
            </CardHeader>
            <CardContent>
              <div class="text-2xl font-bold">{{ inTransitOrders }}</div>
              <p class="text-xs text-muted-foreground">
                {{ inTransitOrdersDelta >= 0 ? '+' : '' }} {{ inTransitOrdersDelta }} since {{ dateRange.start ?
                  dateRange.start.toLocaleDateString() : 'start' }}
              </p>
            </CardContent>
          </Card>

          <Card>
            <CardHeader class="flex flex-row items-center justify-between space-y-0 pb-2">
              <CardTitle class="text-sm font-medium">
                Orders Delivered
              </CardTitle>
              <Icon name="CheckCircle" class="h-4 w-4 text-muted-foreground" />
            </CardHeader>
            <CardContent>
              <div class="text-2xl font-bold">{{ deliveredOrders }}</div>
              <p class="text-xs text-muted-foreground">
                {{ deliveredOrdersDelta >= 0 ? '+' : '' }} {{ deliveredOrdersDelta }} since {{ dateRange.start ?
                  dateRange.start.toLocaleDateString() : 'start' }}
              </p>
            </CardContent>
          </Card>

          <Card>
            <CardHeader class="flex flex-row items-center justify-between space-y-0 pb-2">
              <CardTitle class="text-sm font-medium">
                Alerts
              </CardTitle>
              <Icon name="AlertTriangle" class="h-4 w-4 text-muted-foreground" />
            </CardHeader>
            <CardContent>
              <div class="text-2xl font-bold">{{ complianceAlerts }}</div>
              <p class="text-xs text-muted-foreground">
                {{ complianceAlertsDelta >= 0 ? '+' : '' }} {{ complianceAlertsDelta }} since {{ dateRange.start ?
                  dateRange.start.toLocaleDateString() : 'start' }}
              </p>
            </CardContent>
          </Card>
        </div>

        <div class="grid gap-4 md:grid-cols-1 lg:grid-cols-4">

          <Card class="col-span-1 lg:col-span-3">
            <CardContent>
              <OrderStatusTable tableName="Recent Product Orders" :limit="5" />
            </CardContent>
          </Card>

          <Card>
            <CardContent>
              <RecentNotifications />
            </CardContent>
          </Card>
        </div>

      </TabsContent>

      <!-- EVENTS: live feed + alerts -->
      <TabsContent value="events" class="space-y-4">
        <Card>
          <CardContent>
            <LiveEventFeed />
          </CardContent>
        </Card>
        <Card>
          <CardContent>
            <ComplianceAlerts />
          </CardContent>
        </Card>
      </TabsContent>

      <!-- ANALYTICS: charts -->
      <TabsContent value="analytics" class="space-y-4">
        <Card>
          <CardHeader class="flex items-center justify-between pb-3">
            <CardTitle class="text-lg font-medium">Sensor Data Charts</CardTitle>
          </CardHeader>
          <CardContent>
            <!-- <SensorDataCharts /> -->
          </CardContent>
        </Card>
      </TabsContent>

      <!-- MAP: geo-tracking -->
      <TabsContent value="map" class="space-y-4">
        <Card>
          <CardHeader class="flex items-center justify-between pb-3">
            <CardTitle class="text-lg font-medium">Geo-Tracking Map</CardTitle>
          </CardHeader>
          <CardContent class="h-[400px]">
            <ProductEventMap />
          </CardContent>
        </Card>
      </TabsContent>
    </Tabs>
  </div>
</template>
