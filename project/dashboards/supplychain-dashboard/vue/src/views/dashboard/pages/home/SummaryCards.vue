<script setup lang="ts">
import { ref, onMounted, computed } from 'vue'
import http from '@/utils/http'
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/card'

// Count state
const totalProducts = ref<number>(0)
const totalOrders = ref<number>(0)
const deliveredOrders = ref<number>(0)
const activeTrackers = ref<number>(0)
const productEventsToday = ref<number>(0)
const trackerEventsToday = ref<number>(0)
const eventsToday = ref<number>(0)
const complianceAlerts = ref<number>(0)

// Time window: last 24h
const now = new Date()
const yesterday = new Date(now.getTime() - 24 * 60 * 60 * 1000)
const isoYesterday = computed(() => yesterday.toISOString())

/**
 * Fetch counts from the API
 */
async function fetchCounts() {
  try {
    // Total products
    const prodResp = await http.get<{ count: number }>('/api/products/')
    totalProducts.value = prodResp.data.count

    // Orders and delivered
    const ordResp = await http.get<{ count: number; results: Array<{ current_status: string }> }>(
      '/api/product-orders/?page_size=1000'
    )
    totalOrders.value = ordResp.data.count
    deliveredOrders.value = ordResp.data.results.filter(o => o.current_status === 'delivered').length

    // Active trackers (total trackers)
    const trkResp = await http.get<{ count: number }>('/api/trackers/')
    activeTrackers.value = trkResp.data.count

    // Product events in last 24h
    const peResp = await http.get<{ count: number }>(
      `/api/product-events/?timestamp__gte=${isoYesterday.value}`
    )
    productEventsToday.value = peResp.data.count

    // Tracker events in last 24h
    const teResp = await http.get<{ count: number }>(
      `/api/tracker-events/?timestamp__gte=${isoYesterday.value}`
    )
    trackerEventsToday.value = teResp.data.count

    // total events
    eventsToday.value = productEventsToday.value + trackerEventsToday.value

    // Compliance alerts (placeholder for now)
    complianceAlerts.value = 0
  } catch (e) {
    console.error('Failed to fetch dashboard summary counts', e)
  }
}

onMounted(() => {
  fetchCounts()
})

// Derived in-transit orders
const inTransitOrders = computed(() => totalOrders.value - deliveredOrders.value)
</script>

<template>
  <div class="grid gap-4 md:grid-cols-2 lg:grid-cols-5">
    <Card>
      <CardHeader>
        <CardTitle>Total Products</CardTitle>
      </CardHeader>
      <CardContent>
        <div class="text-2xl font-bold">{{ totalProducts }}</div>
      </CardContent>
    </Card>

    <Card>
      <CardHeader>
        <CardTitle>Total Orders</CardTitle>
      </CardHeader>
      <CardContent>
        <div class="text-2xl font-bold">{{ totalOrders }}</div>
      </CardContent>
    </Card>

    <Card>
      <CardHeader>
        <CardTitle>In-Transit Orders</CardTitle>
      </CardHeader>
      <CardContent>
        <div class="text-2xl font-bold">{{ inTransitOrders }}</div>
      </CardContent>
    </Card>

    <Card>
      <CardHeader>
        <CardTitle>Active Trackers</CardTitle>
      </CardHeader>
      <CardContent>
        <div class="text-2xl font-bold">{{ activeTrackers }}</div>
      </CardContent>
    </Card>

    <Card>
      <CardHeader>
        <CardTitle>Events Last 24h</CardTitle>
      </CardHeader>
      <CardContent>
        <div class="text-2xl font-bold">{{ eventsToday }}</div>
      </CardContent>
    </Card>

    <Card>
      <CardHeader>
        <CardTitle>Compliance Alerts</CardTitle>
      </CardHeader>
      <CardContent>
        <div class="text-2xl font-bold">{{ complianceAlerts }}</div>
      </CardContent>
    </Card>
  </div>
</template>
