<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useForm } from 'vee-validate'
import { toTypedSchema } from '@vee-validate/zod'
import * as z from 'zod'
import http from '@/utils/http'
import { toast } from '@/components/ui/toast'
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { FormField, FormItem, FormLabel, FormControl, FormMessage } from '@/components/ui/form'
import { Input } from '@/components/ui/input'

const router = useRouter()

// Validation schema for IoT Hub message
const formSchema = toTypedSchema(
  z.object({
    deviceID: z.string().min(1, 'Device ID is required'),
    messageType: z.string().min(1, 'Message Type is required'),
    message: z.string().min(1, 'Message payload is required'),
  })
)

// Setup Vee-Validate form
const { handleSubmit, values, resetForm } = useForm({ validationSchema: formSchema })
const loading = ref(false)

// Submit handler
const onSubmit = handleSubmit(async (vals) => {
  loading.value = true
  try {
    await http.post('/api/iot-hub/send/', {
      deviceID: vals.deviceID,
      messageType: vals.messageType,
      message: vals.message,
    })
    toast({ title: 'Message Sent', description: 'Your message was queued successfully.' })
    resetForm()
    // Optionally navigate somewhere
    // router.push({ name: 'dashboard' })
  } catch (err: any) {
    toast({ title: 'Error', description: err.response?.data?.detail || err.message, variant: 'destructive' })
  } finally {
    loading.value = false
  }
})
</script>

<template>
  <Card class="max-w-lg mx-auto mt-6">
    <CardHeader>
      <CardTitle>Send IoT Hub Message</CardTitle>
    </CardHeader>
    <CardContent>
      <form @submit.prevent="onSubmit" class="space-y-4">
        <FormField name="deviceID" v-slot="{ field }">
          <FormItem>
            <FormLabel>Device ID</FormLabel>
            <FormControl>
              <Input v-bind="field" placeholder="e.g. pathledger-gateway-uart-0" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="messageType" v-slot="{ field }">
          <FormItem>
            <FormLabel>Message Type</FormLabel>
            <FormControl>
              <Input v-bind="field" placeholder="e.g. deviceIDUpdate" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="message" v-slot="{ field }">
          <FormItem>
            <FormLabel>Message Payload</FormLabel>
            <FormControl>
              <Input v-bind="field" placeholder="Enter payload (string or number)" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <div class="flex justify-end">
          <Button :loading="loading" type="submit">Send</Button>
        </div>
      </form>
    </CardContent>
  </Card>
</template>
