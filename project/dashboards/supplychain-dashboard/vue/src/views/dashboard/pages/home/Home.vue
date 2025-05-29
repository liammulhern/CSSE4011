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

import { useProductOrderStore } from '@/stores/productorder'
import { useProductEventStore } from '@/stores/productevents'

const auth = useAuthStore()
const orderStore = useProductOrderStore()
const productEventStore = useProductEventStore()
const productOrderStore = useProductOrderStore()

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

onMounted(async () => {
  if (!auth.user) {
    await auth.fetchUser()
  }

  // Fetch summary for the last week by default
  const now = new Date()
  const yesterday = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000)

  dateRange.value = { start: yesterday, end: now }

  await orderStore.fetchSummary(yesterday, now.toISOString())
  await productEventStore.fetchEvents()
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

/**
 * Convert each ProductEvent into a LocationEvent for the map.
 *  - turn lat/lng strings + N/S, E/W into numbers
 *  - build a little HTML snippet for environment & acceleration
 */
const mapMarkers = computed(() => {
  return productEventStore.events
    .map(e => {
      const p = e.payload || {}
      const loc = p.location
      if (!loc || !loc.latitude || !loc.longitude) {
        // can’t place a marker without lat/lng
        return null
      }

      const lat = parseFloat(loc.latitude) * (loc.ns === 'S' ? -1 : 1)
      const lng = parseFloat(loc.longitude) * (loc.ew === 'W' ? -1 : 1)

      // build description only if we have data
      let desc = ''

      if (p.environment) {
        const env = p.environment
        desc += `
          <div><strong>Environment</strong><br/>
            ${env.temperature_c ? `Temp: ${env.temperature_c} °C<br/>` : ''}
            ${env.humidity_percent ? `Humidity: ${env.humidity_percent}%<br/>` : ''}
            ${env.pressure_hpa ? `Pressure: ${env.pressure_hpa} hPa<br/>` : ''}
            ${env.gas_ppm ? `Gas: ${env.gas_ppm}` : ''}
          </div>`
      }

      if (p.acceleration) {
        const a = p.acceleration
        desc += `
          <div style="margin-top:6px;"><strong>Acceleration</strong><br/>
            ${a.x_mps2 ? `X: ${a.x_mps2} m/s²<br/>` : ''}
            ${a.y_mps2 ? `Y: ${a.y_mps2} m/s²<br/>` : ''}
            ${a.z_mps2 ? `Z: ${a.z_mps2} m/s²` : ''}
          </div>`
      }

      return {
        lat,
        lng,
        title: e.event_type,
        description: desc || undefined,
        type: e.event_type,
      }
    })
    .filter((m): m is { lat: number; lng: number; title: string; description?: string; type: string } => m !== null)
})

/**
 * Center the map on the *last* event (or fallback).
 */
const mapCenter = computed<[number, number]>(() => {
  if (mapMarkers.value.length) {
    const last = mapMarkers.value[mapMarkers.value.length - 1]
    return [last.lat, last.lng]
  }
  return [39.5, -95.0]
})

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

      <!-- MAP: geo-tracking -->
      <TabsContent value="map" class="space-y-4">
        <Card>
          <CardHeader class="flex items-start justify-between pb-3">
            <CardTitle class="text-lg font-medium">Geo-Tracking Map</CardTitle>
          </CardHeader>
          <CardContent class="min-h-[500px]">
            <ProductEventMap :event-groups="[]" :events-no-path="mapMarkers" :center="mapCenter" :zoom="12" />
          </CardContent>
        </Card>
      </TabsContent>
    </Tabs>
  </div>
</template>
