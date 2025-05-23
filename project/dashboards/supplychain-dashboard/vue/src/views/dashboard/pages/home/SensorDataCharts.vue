<script setup lang="ts">
import { ref, onMounted, computed } from 'vue'
import http from '@/utils/http'
import { AreaChart } from '@/components/ui/chart-area'
import { LineChart } from '@/components/ui/chart-line'
import { BarChart } from '@/components/ui/chart-bar'
import { DonutChart } from '@/components/ui/chart-donut'

interface Reading {
  timestamp: string
  value: number
}

// Expect props to specify which event type and requirement to chart
const props = defineProps<{ eventType: string; requirementId: number }>()

const requirementDetails = ref<{ min: number; nominal: number; max: number }>({ min: 0, nominal: 0, max: 0 })
const readings = ref<Reading[]>([])
const loading = ref(false)
const error = ref<string | null>(null)

// Computed subsets and transformations
const violations = computed(() =>
  readings.value.filter(
    (r) => r.value < requirementDetails.value.min || r.value > requirementDetails.value.max
  )
)

const areaData = computed(() =>
  readings.value.map((r) => ({
    timestamp: r.timestamp,
    min: requirementDetails.value.min,
    max: requirementDetails.value.max,
  }))
)

const lineData = computed(() =>
  readings.value.map((r) => ({ timestamp: r.timestamp, value: r.value }))
)

const histData = computed(() => {
  const bins: Record<string, number> = {}
  readings.value.forEach((r) => {
    const bin = Math.floor(r.value).toString()
    bins[bin] = (bins[bin] || 0) + 1
  })
  return Object.entries(bins).map(([name, count]) => ({ name, count }))
})

const donutData = computed(() => [
  { name: 'Compliant', total: readings.value.length - violations.value.length },
  { name: 'Violations', total: violations.value.length },
])

/** Fetch requirement details and sensor events */
async function fetchSensorData() {
  loading.value = true
  error.value = null
  try {
    // 1. Get requirement thresholds
    const reqRes = await http.get<{ details: { min: number; nominal: number; max: number } }>(
      `/api/supply-chain-requirements/${props.requirementId}/`
    )
    requirementDetails.value = reqRes.data.details

    // 2. Get product events of this type
    const evRes = await http.get<{ results: any[] }>(
      `/api/product-events/?event_type=${props.eventType}&ordering=timestamp&page_size=1000`
    )
    readings.value = evRes.data.results.map((e) => ({
      timestamp: e.timestamp,
      value: Number(e.payload.value ?? e.payload.reading ?? 0),
    }))
  } catch (e: any) {
    error.value = e.message || String(e)
  } finally {
    loading.value = false
  }
}

onMounted(fetchSensorData)
</script>

<template>
  <div class="space-y-8">
    <div v-if="loading" class="text-center p-4">Loading sensor data…</div>
    <div v-else-if="error" class="text-center p-4 text-red-500">Error: {{ error }}</div>
    <div v-else>
      <div>
        <h3 class="text-lg font-medium mb-2">Threshold Band (Min/Max)</h3>
        <AreaChart :data="areaData" index="timestamp" :categories="['min', 'max']" />
      </div>

      <div>
        <h3 class="text-lg font-medium mb-2">Actual Readings</h3>
        <LineChart :data="lineData" index="timestamp" :categories="['value']"
          :y-formatter="(v) => (typeof v === 'number' ? v.toFixed(2) : '')" />
      </div>

      <div>
        <h3 class="text-lg font-medium mb-2">Reading Distribution</h3>
        <BarChart :data="histData" index="name" :categories="['count']" />
      </div>

      <div>
        <h3 class="text-lg font-medium mb-2">Compliance Ratio</h3>
        <DonutChart index="name" :category="'total'" :data="donutData" />
      </div>
    </div>
  </div>
</template>
