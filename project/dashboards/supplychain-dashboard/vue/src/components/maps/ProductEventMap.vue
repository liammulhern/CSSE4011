<template>
	<MapEvents class="h-full border rounded" :tile-url="tileUrl" :tile-options="tileOpts"
		:event-groups="eventGroups" :events-no-path="standalones" :default-icon="iconDefault"
		:icon-map="iconMap" :path-colors="pathColors" :center="center" :zoom="10" :key="tileUrl" />
</template>
<script setup lang="ts">
import { useAppStore } from '@/stores/app'
import { definePropsm, computed } from 'vue'
import { icon } from 'leaflet'
import MapEvents from '@/components/maps/MapEvents.vue'

import mapPinA from '@/assets/images/map_pin_a.svg'
import mapPinB from '@/assets/images/map_pin_b.svg'
import mapPinC from '@/assets/images/map_pin_c.svg'

interface LocationEvent {
	lat: number
	lng: number
	title: string
	description?: string
	type?: string
}

const {
	eventGroups,
	standalones,
	center
} = defineProps<{
	eventGroups: LocationEvent[][]
	standalones: LocationEvent[]
	center: [number, number]
}>()

// build your icons via Leaflet
const iconDefault = icon({
	iconUrl: mapPinA,
	iconSize: [32, 32],
	iconAnchor: [16, 32],
	popupAnchor: [0, -32],
})

const iconAlert = icon({
	iconUrl: mapPinB,
	iconSize: [32, 32],
	iconAnchor: [16, 32],
	popupAnchor: [0, -32],
})

const iconGood = icon({
	iconUrl: mapPinC,
	iconSize: [32, 32],
	iconAnchor: [16, 32],
	popupAnchor: [0, -32],
})

const iconMap = {
	default: iconDefault,
	alert: iconAlert,
	good: iconGood,
}

const appstore = useAppStore()

const tileUrl = computed(() =>
	appstore.isDark ? darkUrl : lightUrl
)

// CARTO basemap + options
const lightUrl = 'https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png'
const darkUrl = 'https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png'

const tileOpts = {
	attribution:
		'© <a href="https://carto.com/attributions">CARTO</a> © OpenStreetMap contributors',
	subdomains: 'abcd',
	maxZoom: 19,
}

const pathColors = ['orange', 'red', 'green']

</script>
