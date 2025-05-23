<template>
	<MapEvents :tileUrl="tileUrl" :eventGroups="eventGroups" :eventsNoPath="standalones" :defaultIcon="iconDefault"
		:iconMap="iconMap" :events="events" :pathColors="pathColors" :center="[39.5, -95.0]" :zoom="4" />
</template>

<script setup lang="ts">
import { ref } from 'vue';
import MapEvents from '@/components/maps/MapEvents.vue';
import map_pin_a from '@/assets/images/map_pin_a.svg';
import map_pin_b from '@/assets/images/map_pin_b.svg';
import map_pin_c from '@/assets/images/map_pin_c.svg';

const iconDefault = L.icon({
	iconUrl: map_pin_a,
	iconSize: [32, 32],
	iconAnchor: [16, 32],
	popupAnchor: [0, -32],
});

const iconAlert = L.icon({
	iconUrl: map_pin_b,
	iconSize: [32, 32],
	iconAnchor: [16, 32],
	popupAnchor: [0, -32],
});

const iconGood = L.icon({
	iconUrl: map_pin_c,
	iconSize: [32, 32],
	iconAnchor: [16, 32],
	popupAnchor: [0, -32],
});

const iconMap = { default: iconDefault, alert: iconAlert, good: iconGood };

const tileUrl = 'https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png';

const tileOpts = {
	attribution:
		'© <a href="https://carto.com/attributions">CARTO</a> © OpenStreetMap contributors',
	subdomains: 'abcd',
	maxZoom: 19,
};

const eventGroups = ref([
	[
		{ lat: 40.7, lng: -74.0, title: 'Start A', type: 'default' },
		{ lat: 40.8, lng: -73.9, title: 'Middle A', type: 'alert' },
		{ lat: 40.9, lng: -73.8, title: 'End A', type: 'good' },
	],
	[
		{ lat: 34.0, lng: -118.2, title: 'Start B', type: 'default' },
		{ lat: 34.1, lng: -118.1, title: 'End B', type: 'good' },
	],
]);

// markers without any path
const standalones = ref([
	{ lat: 51.5, lng: -0.1, title: 'Solo', description: 'No line', type: 'alert' }
]);

const pathColors = ['orange', 'red', 'green'];

</script>
