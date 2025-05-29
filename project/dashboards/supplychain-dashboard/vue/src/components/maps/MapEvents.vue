<template>
    <LMap ref="mapRef" :center="center" :zoom="zoom" :useGlobalLeaflet="false" style="width: 100%; height: 30rem;">
        <!-- base tiles -->
        <LTileLayer :url="tileUrl" v-bind="tileOptions" />

        <!-- group all markers & paths in a feature-group to fit bounds -->
        <LFeatureGroup ref="fgRef">
            <!-- paths + markers for each eventGroup -->
            <template v-for="(group, idx) in eventGroups" :key="`group-${idx}`">
                <LPolyline :lat-lngs="group.map(e => [e.lat, e.lng])" :pathOptions="{
                    color: pathColors[idx] ?? defaultColors[idx % defaultColors.length],
                    weight: 4
                }" />

                <template v-for="e in group" :key="`${e.title}-${e.lat}-${e.lng}`">
                    <LMarker :lat-lng="[e.lat, e.lng]" :icon="iconMap[e.type] ?? defaultIcon">
                        <LPopup>
                            <strong>{{ e.title }}</strong>
                            <br v-if="e.description" />
                            <span v-if="e.description">{{ e.description }}</span>
                        </LPopup>
                    </LMarker>
                </template>
            </template>

            <!-- standalone events -->
            <template v-for="e in eventsNoPath" :key="`${e.title}-${e.lat}-${e.lng}`">
                <LMarker :lat-lng="[e.lat, e.lng]" :icon="iconMap[e.type] ?? defaultIcon">
                    <LPopup>
                        <div><strong>{{ e.title }}</strong></div>
                        <div v-if="e.description" v-html="e.description"></div>
                    </LPopup>
                </LMarker>
            </template>
        </LFeatureGroup>
    </LMap>
</template>

<script setup lang="ts">
import { ref, watch, onMounted } from 'vue'
import {
    LMap,
    LTileLayer,
    LMarker,
    LPolyline,
    LPopup,
    LFeatureGroup,
} from '@vue-leaflet/vue-leaflet'
import 'leaflet/dist/leaflet.css'

interface LocationEvent {
    lat: number
    lng: number
    title: string
    description?: string
    type?: string
}

const props = defineProps<{
    eventGroups: LocationEvent[][]
    eventsNoPath?: LocationEvent[]
    iconMap?: Record<string, any>
    defaultIcon?: any
    tileUrl?: string
    tileOptions?: Record<string, unknown>
    center?: [number, number]
    zoom?: number
    pathColors?: string[]
}>()

// reactive state
const center = ref(props.center ?? [0, 0])
const zoom = ref(props.zoom ?? 2)
const tileUrl = props.tileUrl ?? 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png'
const tileOptions = props.tileOptions ?? {
    attribution: 'Map data © <a href="https://openstreetmap.org">OpenStreetMap</a> contributors',
}
const eventGroups = props.eventGroups
const eventsNoPath = props.eventsNoPath ?? []
const iconMap = props.iconMap ?? {}
const defaultIcon = props.defaultIcon
const pathColors = props.pathColors ?? []
const defaultColors = ['blue', 'red', 'green', 'orange', 'purple']

// refs to LMap and LFeatureGroup for fitBounds
const mapRef = ref<InstanceType<typeof LMap>>()
const fgRef = ref<InstanceType<typeof LFeatureGroup>>()

// whenever data changes, fit bounds to show everything
watch(
    () => [eventGroups, eventsNoPath],
    () => {
        const map = mapRef.value?.leafletObject
        const fg = fgRef.value?.leafletObject
        if (map && fg && fg.getLayers().length) {
            map.fitBounds(fg.getBounds().pad(0.2))
        }
    },
    { deep: true }
)
</script>
